#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <Adafruit_Fingerprint.h>

// --- Hardware Pins ---
#define SERVO_PIN 18 
#define LED_GREEN 25 
#define LED_RED 26
#define RX_PIN 16
#define TX_PIN 17

// --- Fingerprint ---
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// --- Network ---
const char* ssid = "Aayush hotspot";
const char* password = "987654321";
const char* mqtt_server = "192.168.100.30";
const char* access_topic = "home/security/access";

WiFiClient espClient;
PubSubClient client(espClient);
Servo myServo;

bool sensorReady = false;

void setup() {
 Serial.begin(115200);
 delay(1000);

 Serial.println("\n--- Smart Access System Starting ---");

 // --- Fingerprint Init ---
 mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
 finger.begin(57600);
 delay(500);

 if (finger.verifyPassword()) {
  Serial.println("Fingerprint sensor found!");
  sensorReady = true;
 } else {
  Serial.println("Fingerprint sensor not found.");
 }

 // --- Pins ---
 pinMode(LED_GREEN, OUTPUT);
 pinMode(LED_RED, OUTPUT);
 digitalWrite(LED_GREEN, LOW);
 digitalWrite(LED_RED, LOW);

 myServo.attach(SERVO_PIN);
 myServo.write(0);

 setup_wifi();
 client.setServer(mqtt_server, 1883);
}

void loop() {
 if (!client.connected()) {
  reconnect();
 }
 client.loop();

 if (sensorReady) {
  checkFingerprint();
 }
}

// ================== FINGERPRINT ==================

void checkFingerprint() {
 uint8_t p = finger.getImage();
 if (p != FINGERPRINT_OK) return;

 p = finger.image2Tz();
 if (p != FINGERPRINT_OK) return;

 p = finger.fingerFastSearch();

 if (p == FINGERPRINT_OK) {
  Serial.print("Match found! ID: ");
  Serial.println(finger.fingerID);
  handleAccess(true, finger.fingerID);
 } else {
  Serial.println("No match found!");
  handleAccess(false, 0);
 }

 // Wait until finger is removed (prevents spam triggers)
 while (finger.getImage() != FINGERPRINT_NOFINGER);
}

// ================== ACCESS HANDLER ==================

// Helper function to map IDs to Names
String getUserName(int id) {
 switch (id) {
  case 1: return "Aayush Dhakal";
  case 2: return "Sabin Shrestha";
  case 4: return "Ashutosh Man Singh";
  default: return "Unknown User";
 }
}

void handleAccess(bool granted, int id) {
 if (granted) {
  digitalWrite(LED_GREEN, HIGH);
  myServo.write(90);

  // Get the name based on the ID
  String name = getUserName(id);

  // Updated JSON to include the "user" name
  String payload = "{\"status\":\"Authorized\",\"id\":" + String(id) + ",\"user\":\"" + name + "\"}";
  client.publish(access_topic, payload.c_str());

  Serial.print("Access Granted to: ");
  Serial.println(name);

  delay(3000);

  myServo.write(0);
  digitalWrite(LED_GREEN, LOW);
 } 
 else {
  digitalWrite(LED_RED, HIGH);

  String payload = "{\"status\":\"Denied\",\"user\":\"Unknown\"}";
  client.publish(access_topic, payload.c_str());

  Serial.println("Access Denied!");

  delay(1500);
  digitalWrite(LED_RED, LOW);
 }

 delay(2000); // debounce delay
}

// ================== WIFI ==================

void setup_wifi() {
 Serial.print("Connecting to WiFi");

 WiFi.begin(ssid, password);

 int retries = 20;
 while (WiFi.status() != WL_CONNECTED && retries--) {
  delay(500);
  Serial.print(".");
 }

 if (WiFi.status() == WL_CONNECTED) {
  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
 } else {
  Serial.println("\nWiFi failed");
 }
}

// ================== MQTT ==================

void reconnect() {
 while (!client.connected()) {
  Serial.print("Connecting to MQTT...");

  String clientId = "ESP32_Security_";
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str())) {
   Serial.println("Connected");
  } else {
   Serial.print("Failed, rc=");
   Serial.print(client.state());
   Serial.println(" retrying in 5s");
   delay(5000);
  }
 }
}
