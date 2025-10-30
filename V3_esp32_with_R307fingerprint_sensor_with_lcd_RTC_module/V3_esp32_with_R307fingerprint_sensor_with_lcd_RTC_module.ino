#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>

// --- Device Setups ---
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

// --- State Management ---
bool enrolled = false;

void setup() {
  Serial.begin(115220);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Setting Time...");
  Serial.println("System Booting... Attempting to set RTC time.");

  if (!rtc.begin()) {
    Serial.println("❌ Couldn't find RTC! Halting.");
    lcd.clear();
    lcd.print("RTC Error!");
    while (1);
  }

  // ===================================================================
  // --- CRITICAL STEP: THIS LINE SETS THE REAL-TIME CLOCK ---
  // This line is currently ACTIVE. When you upload this code, it will
  // set the RTC module to the exact date and time your computer has
  // at the moment of compilation.
  
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  // AFTER THIS UPLOAD IS SUCCESSFUL, YOU MUST:
  // 1. Comment out the line above like this: // rtc.adjust(...);
  // 2. Upload the code a SECOND time.
  // This will prevent the clock from being reset every time the device reboots.
  // ===================================================================

  Serial.println("✅ RTC Time has been set.");
  lcd.clear();
  lcd.print("Time Set!");
  delay(1500);

  // --- Initialize Fingerprint Sensor ---
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);
  delay(1000);

  if (!finger.verifyPassword()) {
    lcd.clear();
    lcd.print("Sensor Error!");
    Serial.println("❌ Fingerprint sensor not found!");
    while (1);
  }

  Serial.println("✅ R307 detected successfully!");
  lcd.clear();
  lcd.print("System Ready");
  delay(1000);
}

void loop() {
  DateTime now = rtc.now();
  char timeBuffer[9];
  sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  lcd.setCursor(8, 0);
  lcd.print(timeBuffer);
  lcd.setCursor(0, 0);

  if (!enrolled) {
    lcd.print("Enroll #1  ");
    if (getFingerprintEnroll(1, timeBuffer) == FINGERPRINT_OK) {
      lcd.clear();
      lcd.print("Enroll Success!");
      Serial.println("✅ Finger enrolled successfully!");
      delay(2000);
      enrolled = true;
    } else {
      lcd.clear();
      lcd.print("Enroll Failed");
      Serial.println("❌ Enrollment failed.");
      delay(2000);
    }
  } else {
    lcd.print("Verify...  ");
    getFingerprintID();
    delay(500);
  }
}

// Full helper function for enrolling a finger
uint8_t getFingerprintEnroll(uint8_t id, const char* timeBuffer) {
  int p = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place Finger");
  lcd.setCursor(0, 1);
  lcd.print("to Enroll...");
  Serial.println("\nPlace your finger on the sensor to enroll...");

  while (p != FINGERPRINT_OK) {
    lcd.setCursor(8, 0);
    lcd.print(timeBuffer); // Update time while waiting
    p = finger.getImage();
  }

  Serial.println("Image taken.");
  lcd.clear();
  lcd.print("Image Captured!");
  delay(1000);
  
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) { lcd.clear(); lcd.print("Imaging Error"); delay(1000); return p; }
  Serial.println("Image converted.");

  lcd.clear();
  lcd.print("Remove Finger");
  Serial.println("Remove your finger.");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  lcd.clear();
  lcd.print("Place Finger");
  lcd.setCursor(0, 1);
  lcd.print("Again...");
  Serial.println("Place the same finger again.");
  
  while (finger.getImage() != FINGERPRINT_OK);
  Serial.println("Second image taken.");
  
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) { lcd.clear(); lcd.print("Imaging Error"); delay(1000); return p; }
  Serial.println("Second image converted.");

  lcd.clear();
  lcd.print("Creating Model...");
  Serial.println("Creating model...");
  p = finger.createModel();
  if (p != FINGERPRINT_OK) { lcd.clear(); lcd.print("Model Error"); delay(1000); return p; }
  
  Serial.println("Storing model...");
  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) { lcd.clear(); lcd.print("Storage Error"); delay(1000); return p; }

  return FINGERPRINT_OK;
}

// Full helper function for verifying a finger
int getFingerprintID() {
  uint8_t p = finger.getImage();
  
  if (p == FINGERPRINT_NOFINGER) {
    lcd.setCursor(0,0);
    lcd.print("Place Finger ");
    lcd.setCursor(0,1);
    lcd.print("to Verify... ");
    return -1;
  }
  
  Serial.println("Finger detected, verifying...");
  lcd.clear();
  lcd.print("Verifying...");

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Imaging Error");
    Serial.println("❌ Imaging error.");
    delay(1500);
    return -1;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    DateTime now = rtc.now();
    char timestampBuffer[20];
    sprintf(timestampBuffer, "%04d-%02d-%02d %02d:%02d:%02d",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
            
    Serial.print("✅ Match found! Welcome ID #");
    Serial.print(finger.fingerID);
    Serial.print(" at ");
    Serial.println(timestampBuffer);

    lcd.clear();
    lcd.print("Access Granted!");
    lcd.setCursor(0, 1);
    lcd.print("Welcome ID: #" + String(finger.fingerID));
    
    delay(3000);
    return finger.fingerID;
  } else {
    lcd.clear();
    lcd.print("Access Denied");
    lcd.setCursor(0, 1);
    lcd.print("Try Again");
    Serial.println("❌ No match found.");
    delay(2000);
    return -1;
  }
}