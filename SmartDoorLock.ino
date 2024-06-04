#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int pinBuzz = 10;
const int doorLock = 11;
const int limitSwitchPin = 12;
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27, 16, 2); // Ensure using correct I2C address

String storedData = "6B2CD"; // Correct data for input keypad
String inputData = ""; // Set none input keypad

bool wasDoorClosed = true;
int incorrectAttempts = 0;
unsigned long lockoutStartTime = 0;
const unsigned long lockoutDuration = 15000; // 15s in milliseconds

void setup() {
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Code:");
  pinMode(pinBuzz, OUTPUT);
  pinMode(doorLock, OUTPUT);
  pinMode(limitSwitchPin, INPUT_PULLUP); // Activation internal resistor pull-up
  digitalWrite(doorLock, LOW);
}

void loop() {
  // Check the status of the limit switch
  bool isDoorClosed = digitalRead(limitSwitchPin) == HIGH;
  char key = keypad.getKey();

  // Check if lockout period is active
  if (incorrectAttempts >= 3) {
    unsigned long elapsedLockoutTime = millis() - lockoutStartTime;
    if (elapsedLockoutTime < lockoutDuration) {
      // Display lockout timer on LCD
      unsigned long remainingTime = lockoutDuration - elapsedLockoutTime;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Locked for:");
      lcd.setCursor(0, 1);
      lcd.print(remainingTime / 1000);
      lcd.print(" sec");
      delay(1000);
      return;
    } else {
      // Reset incorrect attempts after lockout period ends
      incorrectAttempts = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter Code:");
    }
  }

  if (isDoorClosed) {
    // If the door was just closed, lock it again and reset input
    if (!wasDoorClosed) {
      digitalWrite(doorLock, LOW); // Lock the door again
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter Code:");
      inputData = ""; // Reset input data to ensure no leftover input
      delay(1000); // Ensure enough time to reset display
    }

    // Only proceed if the door is closed
    if (key) {
      tone(pinBuzz, 3000);
      delay(150);
      noTone(pinBuzz);
      delay(150);

      if (key == '#') { // If the '#' key is pressed, check the input
        if (inputData == storedData) { // If the input matches the stored data
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Access Accepted");
          tone(pinBuzz, 3000);
          delay(2000);
          noTone(pinBuzz);
          digitalWrite(doorLock, HIGH); // Unlock the door
          lcd.clear();
          lcd.print("Enter Code:");
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Incorrect PIN");
          for (int i = 0; i < 4; i++) { // Sound the buzzer 4 times
            tone(pinBuzz, 3000);
            delay(500);
            noTone(pinBuzz);
            delay(500);
          }
          delay(2000);
          lcd.clear();
          lcd.print("Enter Code:");
          incorrectAttempts++;
          if (incorrectAttempts >= 3) {
            lockoutStartTime = millis();
          }
        }

        inputData = ""; // Reset input data

      } else if (key == '*') { // If the '*' key is pressed, delete the input
        inputData = "";
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter Code:");
      } else {
        inputData += key; // Add keypad input to inputData
        lcd.setCursor(0, 1);
        lcd.print(inputData);
      }
    }
  } else {
    // If the door is open, only display "Enter Code:" on the first line
    lcd.setCursor(0, 0);
    lcd.print("Enter Code:");
    delay(1000); // Add a delay to avoid excessive LCD flickering
  }

  // Update the previous door status
  wasDoorClosed = isDoorClosed;
}