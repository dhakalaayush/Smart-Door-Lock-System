#include <Adafruit_Fingerprint.h>

HardwareSerial mySerial(2);

#define RX_PIN 16
#define TX_PIN 17

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

int id;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nFingerprint Enroll (ESP32)");

  mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Sensor found!");
  } else {
    Serial.println("Sensor NOT found");
    while (1) { delay(1); }
  }

  Serial.println("\nEnter ID (1 - 127) to save fingerprint:");
}

void loop() {
  if (Serial.available()) {
    id = Serial.parseInt();
    if (id >= 1 && id <= 127) {
      Serial.print("Enrolling ID #");
      Serial.println(id);
      enrollFingerprint(id);
    } else {
      Serial.println("Invalid ID. Enter 1–127.");
    }

    // Ask again after one enrollment
    Serial.println("\nEnter another ID:");
  }
}

void enrollFingerprint(int id) {
  int p = -1;

  Serial.println("Place finger on sensor...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  Serial.println("Image taken");

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error converting image");
    return;
  }

  Serial.println("Remove finger...");
  delay(2000);

  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  Serial.println("Place SAME finger again...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  Serial.println("Image taken again");

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error converting second image");
    return;
  }

  Serial.println("Creating model...");

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Fingerprints did not match");
    return;
  }

  Serial.println("Fingerprint matched!");

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored successfully!");
  } else {
    Serial.println("Error storing fingerprint");
  }
}
