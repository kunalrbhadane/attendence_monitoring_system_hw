#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

// --- Device Setups ---
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
const int chipSelectPin = 5;
File attendanceFile;

// --- State Machine ---
enum AppState {
  WAITING_FOR_ENROLL,
  ENROLLING_STEP_1,
  ENROLLING_STEP_2,
  VERIFYING
};
AppState currentState = WAITING_FOR_ENROLL;
uint8_t id_to_enroll = 1;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("System Booting...");

  if (!rtc.begin()) {
    Serial.println("❌ RTC Error!"); lcd.clear(); lcd.print("RTC Error!"); while (1);
  }
  
  // --- IMPORTANT ---
  // On first upload, uncomment the line below to set the time.
  // Then, re-comment it and upload again.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  if (!SD.begin(chipSelectPin)) {
    Serial.println("❌ SD Card Error!"); lcd.clear(); lcd.print("SD Card Error!"); while (1);
  }
  
  if (!SD.exists("/attendance.csv")) {
    attendanceFile = SD.open("/attendance.csv", FILE_WRITE);
    if (attendanceFile) {
      attendanceFile.println("Finger_ID,Date,Time");
      attendanceFile.close();
    }
  }

  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  if (!finger.verifyPassword()) {
    Serial.println("❌ Sensor Error!"); lcd.clear(); lcd.print("Sensor Error!"); while (1);
  }

  Serial.println("\n✅ System Ready. Waiting to enroll ID #1...");
  lcd.clear();
  lcd.print("System Ready");
  delay(1500);
}

void loop() {
  displayTime();
  
  switch (currentState) {
    case WAITING_FOR_ENROLL:
      handle_waiting_for_enroll();
      break;
    case ENROLLING_STEP_1:
      handle_enroll_step_1();
      break;
    case ENROLLING_STEP_2:
      handle_enroll_step_2();
      break;
    case VERIFYING:
      handle_verifying();
      break;
  }
}

// --- State Handler Functions ---

void handle_waiting_for_enroll() {
  lcd.setCursor(0, 0);
  lcd.print("Place Finger");
  lcd.setCursor(0, 1);
  lcd.print("to Enroll ID #1");
  
  if (finger.getImage() == FINGERPRINT_OK) {
    Serial.println("Image 1 taken for enrollment.");
    lcd.clear(); lcd.print("Image Captured!");
    delay(1000);

    if (finger.image2Tz(1) != FINGERPRINT_OK) {
      lcd.clear(); lcd.print("Imaging Error"); delay(1500);
      currentState = WAITING_FOR_ENROLL; return;
    }
    
    currentState = ENROLLING_STEP_1;
    lcd.clear(); lcd.print("Remove Finger");
    Serial.println("Remove your finger.");
    delay(1000);
  }
}

void handle_enroll_step_1() {
  lcd.setCursor(0, 0);
  lcd.print("Remove Finger");
  
  if (finger.getImage() == FINGERPRINT_NOFINGER) {
    Serial.println("Finger removed.");
    currentState = ENROLLING_STEP_2;
    lcd.clear(); lcd.print("Place Again...");
    Serial.println("Place the same finger again.");
    delay(1000);
  }
}

void handle_enroll_step_2() {
  lcd.setCursor(0, 0);
  lcd.print("Place Finger");
  lcd.setCursor(0, 1);
  lcd.print("Again...");

  if (finger.getImage() == FINGERPRINT_OK) {
    Serial.println("Image 2 taken for enrollment.");
    lcd.clear(); lcd.print("Image Captured!");
    delay(1000);

    if (finger.image2Tz(2) != FINGERPRINT_OK) {
      lcd.clear(); lcd.print("Imaging Error"); delay(1500);
      currentState = WAITING_FOR_ENROLL; return;
    }

    lcd.clear(); lcd.print("Creating Model...");
    if (finger.createModel() != FINGERPRINT_OK) {
      lcd.clear(); lcd.print("Model Error"); delay(1500);
      currentState = WAITING_FOR_ENROLL; return;
    }

    if (finger.storeModel(id_to_enroll) != FINGERPRINT_OK) {
      lcd.clear(); lcd.print("Storage Error"); delay(1500);
      currentState = WAITING_FOR_ENROLL; return;
    }

    Serial.println("✅ Enrollment successful!");
    lcd.clear(); lcd.print("Enroll Success!");
    delay(2000);
    currentState = VERIFYING;
  }
}

void handle_verifying() {
  lcd.setCursor(0, 0);
  lcd.print("Place Finger");
  lcd.setCursor(0, 1);
  lcd.print("to Verify...");

  getFingerprintIDandLog(); 
  
  // ====================== SOFTWARE FIX IS HERE ======================
  // Add a significant delay in the verifying state. This reduces the
  // frequency of the power-hungry scanning operation and gives the
  // power supply time to stabilize between attempts.
  delay(1000);
  // ================================================================
}


// --- Helper Functions ---

void displayTime() {
  DateTime now = rtc.now();
  char timeBuffer[9];
  sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  lcd.setCursor(8, 0);
  lcd.print(timeBuffer);
}

void getFingerprintIDandLog() {
  if (finger.getImage() != FINGERPRINT_OK) {
    return;
  }

  lcd.clear();
  lcd.print("Verifying...");

  if (finger.image2Tz() != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("Imaging Error"); delay(1500); return;
  }
  
  if (finger.fingerSearch() == FINGERPRINT_OK) {
    lcd.clear(); lcd.print("Access Granted!");
    lcd.setCursor(0, 1);
    lcd.print("Welcome ID: #" + String(finger.fingerID));
    
    DateTime now = rtc.now();
    char dateBuffer[11], timeBuffer[9];
    sprintf(dateBuffer, "%04d-%02d-%02d", now.year(), now.month(), now.day());
    sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    String dataString = String(finger.fingerID) + "," + dateBuffer + "," + timeBuffer;

    attendanceFile = SD.open("/attendance.csv", FILE_APPEND);
    if (attendanceFile) {
      attendanceFile.println(dataString);
      attendanceFile.close();
      Serial.println("✅ Data saved: " + dataString);
    } else {
      Serial.println("❌ SD Write Error.");
      lcd.clear(); lcd.print("SD Write Error!");
    }
    delay(2000);
  } else {
    lcd.clear(); lcd.print("Access Denied");
    delay(2000);
  }
}