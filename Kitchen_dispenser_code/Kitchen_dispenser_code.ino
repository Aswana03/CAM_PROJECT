#include "HX711.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include <ESP32Servo.h>

// Initialize HX711
HX711 scale;
uint8_t dataPin = 25;
uint8_t clockPin = 32;

// Initialize the I2C LCD (Address 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {19, 18, 5, 17};
byte colPins[COLS] = {16, 4, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servo motors
Servo servoContainer;
Servo servo1;
Servo servo2;

void setup() {
  Wire.begin(21, 22); // Set ESP32 I2C pins
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  resetSystem();

  // Initialize HX711
  scale.begin(dataPin, clockPin);
  scale.set_scale(349.67); // Adjust calibration factor
  scale.tare(); // Reset scale to zero

  // Attach servos
  servoContainer.attach(26);
  servo1.attach(13);
  servo2.attach(27);

  // Set initial servo positions
  servoContainer.write(90);
  servo1.write(0);
  servo2.write(0);
}

void moveServoSmoothly(Servo &servo, int targetAngle) {
  int currentAngle = servo.read();
  int step = (targetAngle > currentAngle) ? 1 : -1;
  for (int angle = currentAngle; angle != targetAngle; angle += step) {
    servo.write(angle);
    delay(10);
  }
  servo.write(targetAngle);
}

void resetSystem() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Please select");
  lcd.setCursor(0, 1);
  lcd.print("an item:");
}

float getAverageWeight() {
  float totalWeight = 0;
  for (int i = 0; i < 200; i++) { // Taking 200 readings for accuracy
    totalWeight += scale.get_units(1);
    delay(5); // Short delay to balance speed and stability
  }
  return totalWeight / 200; // Return the average weight
}

void loop() {
  static String quantity = "";
  static bool selectingItem = true;
  static int selectedItem = 0;

  char key = keypad.getKey();
  
  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);
    
    if (selectingItem) {
      lcd.clear();
      lcd.setCursor(0, 0);
      
      switch (key) {
        case '1': lcd.print("Item 1 Selected"); selectedItem = 1; break;
        case '2': lcd.print("Item 2 Selected"); selectedItem = 2; break;
        default:
          lcd.print("Invalid Key");
          delay(2000);
          resetSystem();
          return;
      }
      
      Serial.print("Item Selected: ");
      Serial.println(key);
      
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter quantity");
      lcd.setCursor(0, 1);
      lcd.print("(kg): ");
      selectingItem = false;
      quantity = "";
    } else {
      if ((key >= '0' && key <= '9') || key == '*') {
        if (key == '*') key = '.';
        quantity += key;
        lcd.setCursor(6, 1);
        lcd.print(quantity + " kg ");
      } else if (key == '#') {
        float qty = quantity.toFloat();
        if (qty > 5) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Enter less than");
          lcd.setCursor(0, 1);
          lcd.print("5kg");
          delay(2000);
          resetSystem();
          return;
        }

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Processing...");
        moveServoSmoothly(servoContainer, 360);
        delay(3000);

        if (selectedItem == 1) moveServoSmoothly(servo1, 90);
        else if (selectedItem == 2) moveServoSmoothly(servo2, 90);
        delay(2000);

        // Read and display weight 3 times
        for (int i = 0; i < 3; i++) {
          float weight = getAverageWeight();
          Serial.print("Measured Weight: ");
          Serial.println(weight);
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Weight: ");
          lcd.setCursor(0, 1);
          lcd.print(weight);
          lcd.print(" g");
          delay(3000);
        }

        // Reset servos
        if (selectedItem == 1) moveServoSmoothly(servo1, 0);
        else if (selectedItem == 2) moveServoSmoothly(servo2, 0);
        delay(2000);
        moveServoSmoothly(servoContainer, 90);
        delay(3000);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Thank You!");
        delay(2000);

        selectingItem = true;
        resetSystem();
      }
    }
  }
}
