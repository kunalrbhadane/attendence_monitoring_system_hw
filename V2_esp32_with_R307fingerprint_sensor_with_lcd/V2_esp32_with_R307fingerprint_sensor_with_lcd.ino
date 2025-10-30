#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <LiquidCrystal_I2C.h> // The new library for the LCD

// --- Fingerprint Sensor Setup (same as Step 1) ---
HardwareSerial mySerial(2);  // UART2 for ESP32
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// --- New LCD Setup ---
// The address (0x27) is the most common one. If your screen is blank,
// it might be 0x3F. You can use an "I2C Scanner" sketch to find the correct address.
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// --- State Management ---
bool enrolled = false;

void setup() {
  Serial.begin(115200); // We still use this for debugging

  // --- LCD Initialization ---
  lcd.init();      // Initialize the lcd 
  lcd.backlight(); // Turn on the backlight
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Fingerprint");
  lcd.setCursor(0, 1);
  lcd.print("System Booting...");
  
  // --- Fingerprint Sensor Initialization (same as Step 1) ---
  mySerial.begin(57600, SERIAL_8N1, 16, 17); // RX, TX pins for ESP32 UART2
  delay(1000); // Give sensor time to boot

  Serial.println("R307 Fingerprint Enrollment + Verification");

  finger.begin(57600);
  delay(1000);

  if (finger.verifyPassword()) {
    Serial.println("✅ R307 detected successfully!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Ready!");
    delay(2000);
  } else {
    Serial.println("❌ Fingerprint sensor not found!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error!");
    lcd.setCursor(0, 1);
    lcd.print("Check Wiring");
    while (1); // Halt on error
  }
}

void loop() {
  if (!enrolled) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enroll New");
    lcd.setCursor(0, 1);
    lcd.print("Finger... ID #1");
    delay(2000);

    // Call the enrollment function
    if (getFingerprintEnroll(1) == FINGERPRINT_OK) {
      lcd.clear();
      lcd.print("Enroll Success!");
      delay(2000);
      enrolled = true; // Set state to enrolled
    } else {
      lcd.clear();
      lcd.print("Enroll Failed");
      delay(2000);
    }
  } else {
    // If already enrolled, continuously check for a finger to verify
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Place Finger");
    lcd.setCursor(0, 1);
    lcd.print("to Verify...");
    
    getFingerprintID();
    delay(1000); // Wait a second between verification attempts
  }
}

//---------------- Enroll ----------------
uint8_t getFingerprintEnroll(uint8_t id) {
  int p = -1;
  lcd.clear();
  lcd.print("Waiting for");
  lcd.setCursor(0, 1);
  lcd.print("Valid Finger...");
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) { lcd.print("Imaging Error"); delay(1000); return p; }
  lcd.clear();
  lcd.print("Image Captured!");
  delay(1000);
  
  lcd.clear();
  lcd.print("Remove Finger");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  lcd.clear();
  lcd.print("Place Finger");
  lcd.setCursor(0, 1);
  lcd.print("Again...");
  
  while (finger.getImage() != FINGERPRINT_OK);

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) { lcd.print("Imaging Error"); delay(1000); return p; }
  
  lcd.clear();
  lcd.print("Creating Model...");

  p = finger.createModel();
  if (p != FINGERPRINT_OK) { lcd.print("Model Error"); delay(1000); return p; }

  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) { lcd.print("Storage Error"); delay(1000); return p; }

  return FINGERPRINT_OK;
}

//---------------- Verify ----------------
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) return -1; // No finger, just return

  // If a finger is present...
  lcd.clear();
  lcd.print("Verifying...");

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Imaging Error");
    return -1;
  }

  p = finger.fingerSearch(); // Changed from fingerFastSearch for better reliability
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Access Granted!");
    lcd.setCursor(0, 1);
    lcd.print("Welcome ID: #" + String(finger.fingerID));
    delay(3000); // Show success message
    return finger.fingerID;
  } else {
    lcd.clear();
    lcd.print("Access Denied");
    lcd.setCursor(0, 1);
    lcd.print("Try Again");
    delay(2000); // Show failure message
    return -1;
  }
}