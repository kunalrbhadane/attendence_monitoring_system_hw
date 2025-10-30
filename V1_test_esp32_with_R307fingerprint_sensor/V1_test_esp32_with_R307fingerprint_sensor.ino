// It enrolls one fingerprint (ID #1)
// Then switches to verification mode (no more repeating)
// LED on sensor will light automatically during scans

#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

HardwareSerial mySerial(2);  // UART2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

bool enrolled = false;

void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  delay(2000);

  Serial.println("R307 Fingerprint Enrollment + Verification");

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("✅ R307 detected successfully!");
  } else {
    Serial.println("❌ Fingerprint sensor not found!");
    while (1);
  }

  Serial.println("\nPlace your finger to enroll...");
}

void loop() {
  if (!enrolled) {
    if (getFingerprintEnroll(1) == FINGERPRINT_OK) {
      Serial.println("✅ Finger enrolled successfully!");
      enrolled = true;
      Serial.println("\nNow place the same finger to verify...");
      delay(3000);
    }
  } else {
    getFingerprintID();
    delay(2000);
  }
}

//---------------- Enroll ----------------
uint8_t getFingerprintEnroll(uint8_t id) {
  int p = -1;
  Serial.println("Waiting for finger...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
  }
  Serial.println("Image taken");

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return p;
  Serial.println("Image converted");

  Serial.println("Remove finger");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println("Place same finger again...");
  while (finger.getImage() != FINGERPRINT_OK);
  Serial.println("Image taken");

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return p;

  p = finger.createModel();
  if (p != FINGERPRINT_OK) return p;

  p = finger.storeModel(id);
  return p;
}

//---------------- Verify ----------------
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("✅ Match found! ID #");
    Serial.println(finger.fingerID);
    return finger.fingerID;
  } else {
    Serial.println("❌ No match found.");
    return -1;
  }
}
