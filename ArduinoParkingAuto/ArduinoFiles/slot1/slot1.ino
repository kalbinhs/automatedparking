#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <MFRC522.h>

// Wifi and Firebase Setup
#define WIFI_SSID "lippyphone"
#define WIFI_PASSWORD "garamlaut"
#define API_KEY "AIzaSyB6EvUkC7g8MAUdbQG5nkB1MPY0lT4sdbE"
#define DATABASE_URL "arduinoparkingauto-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "testemail@gmail.com"
#define USER_PASSWORD "123123"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// RFID Scanner Setup
#define SS_PIN D2
#define RST_PIN D1
MFRC522 rfid(SS_PIN, RST_PIN);

char scannedRFIDString[9];

// Ultrasonic Sensor Setup
#define TRIG_PIN D3
#define ECHO_PIN D4

char state[50];

#define buzzer D0

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  if (!rfid.PCD_PerformSelfTest()) {
    Serial.println("Initialization failed. Please check your connections and restart.");
  }

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(buzzer, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  Firebase.begin(&config, &auth);

  Firebase.setDoubleDigits(5);

  updateState();
  beep(2000, 100);
  delay(10);
  beep(2000, 100);
}

void loop() {  
  if (strcmp(state, "WAIT_GATE") == 0) {
    // Handle WAIT_GATE state
    updateState();
    delay(3000);
  } else if (strcmp(state, "PARK_CAR") == 0) {
      // Handle PARK_CAR state
      if (detectObject()) {
        beepSuccess();
        strcpy(state, "SCAN_SLOT");
        Serial.printf("Set state... %s\n", Firebase.setString(fbdo, F("/slot1/state"), "SCAN_SLOT") ? "SCAN_SLOT" : fbdo.errorReason().c_str());
      }
  } else if (strcmp(state, "SCAN_SLOT") == 0) {
      // Handle SCAN_SLOT state
      if(scanRFID()) {
        byte scannedRFID[] = {rfid.uid.uidByte[0], rfid.uid.uidByte[1], rfid.uid.uidByte[2], rfid.uid.uidByte[3]};
        sprintf(scannedRFIDString, "%02X%02X%02X%02X", scannedRFID[0], scannedRFID[1], scannedRFID[2], scannedRFID[3]);
        Serial.printf("Current RFID : %s\n", scannedRFIDString);

        if (strcmp(Firebase.getString(fbdo, F("/slot1/card")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str(), scannedRFIDString) == 0) {
          beepSuccess();
          strcpy(state, "LEAVE_CAR");
          Serial.printf("Set state... %s\n", Firebase.setString(fbdo, F("/slot1/state"), "LEAVE_CAR") ? "LEAVE_CAR" : fbdo.errorReason().c_str());
        } else {
          beepWarning();
          strcpy(state, "WARNING_ONE");
          Serial.printf("Set state... %s\n", Firebase.setString(fbdo, F("/slot1/state"), "WARNING_ONE") ? "WARNING_ONE" : fbdo.errorReason().c_str());
        }
      }
  } else if (strcmp(state, "LEAVE_CAR") == 0) {
      // Handle LEAVE_CAR state
      if (detectLeave()) {
        beepSuccess();
        strcpy(state, "WAIT_GATE");
        Serial.printf("Set state... %s\n", Firebase.setString(fbdo, F("/slot1/state"), "WAIT_GATE") ? "WAIT_GATE" : fbdo.errorReason().c_str());
        Serial.printf("Card emptied... %s\n", Firebase.setString(fbdo, F("/slot1/card"), "empty") ? "ok" : fbdo.errorReason().c_str());
      }
  } else if (strcmp(state, "WARNING_ONE") == 0) {
      // Handle WARNING_ONE state
      if(scanRFID()) {
        byte scannedRFID[] = {rfid.uid.uidByte[0], rfid.uid.uidByte[1], rfid.uid.uidByte[2], rfid.uid.uidByte[3]};
        sprintf(scannedRFIDString, "%02X%02X%02X%02X", scannedRFID[0], scannedRFID[1], scannedRFID[2], scannedRFID[3]);
        Serial.printf("Current RFID : %s\n", scannedRFIDString);

        if (strcmp(Firebase.getString(fbdo, F("/slot1/card")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str(), scannedRFIDString) == 0) {
          beepSuccess();
          strcpy(state, "LEAVE_CAR");
          Serial.printf("Set state... %s\n", Firebase.setString(fbdo, F("/slot1/state"), "LEAVE_CAR") ? "LEAVE_CAR" : fbdo.errorReason().c_str());
        } else {
          beepWarning();
          strcpy(state, "WARNING_TWO");
          Serial.printf("Set state... %s\n", Firebase.setString(fbdo, F("/slot1/state"), "WARNING_TWO") ? "WARNING_TWO" : fbdo.errorReason().c_str());
        }
      }
  } else if (strcmp(state, "WARNING_TWO") == 0) {
      // Handle WARNING_TWO state
      if(scanRFID()) {
        byte scannedRFID[] = {rfid.uid.uidByte[0], rfid.uid.uidByte[1], rfid.uid.uidByte[2], rfid.uid.uidByte[3]};
        sprintf(scannedRFIDString, "%02X%02X%02X%02X", scannedRFID[0], scannedRFID[1], scannedRFID[2], scannedRFID[3]);
        Serial.printf("Current RFID : %s\n", scannedRFIDString);

        if (strcmp(Firebase.getString(fbdo, F("/slot1/card")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str(), scannedRFIDString) == 0) {
          beepSuccess();
          strcpy(state, "LEAVE_CAR");
          Serial.printf("Set state... %s\n", Firebase.setString(fbdo, F("/slot1/state"), "LEAVE_CAR") ? "LEAVE_CAR" : fbdo.errorReason().c_str());
        } else {
          beepFail();
          strcpy(state, "ADMIN_RESET");
          Serial.printf("Set state... %s\n", Firebase.setString(fbdo, F("/slot1/state"), "ADMIN_RESET") ? "ADMIN_RESET" : fbdo.errorReason().c_str());
        }
      }
  } else if (strcmp(state, "ADMIN_RESET") == 0) {
      // Handle ADMIN_RESET state
      updateState();
      delay(3000);
  } else {
      Serial.println("State error");
      delay(3000);
  }
}

void updateState() {
    strcpy(state, Firebase.getString(fbdo, F("/slot1/state")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str());
    Serial.println("State updated");
}

bool scanRFID() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    return true;
  }
  return false;
}

bool detectObject() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;

  return (distance <= 15 && distance >= 5);
}

bool detectLeave() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;

  return (distance >= 30);
}

void beep(int frequency, int duration_ms)
{
  tone(buzzer, frequency);
  delay(duration_ms);
  noTone(buzzer);
}

void beepSuccess(){
  beep(500, 100);
  delay(50);
  beep(1000, 100);
  delay(50);
  beep(1500, 100);
  delay(50);
  beep(2000, 100);
}

void beepWarning(){
  beep(700, 100);
  delay(50);
  beep(500, 100);
  delay(50);
  beep(300, 100);
}

void beepFail(){
  beep(500, 1000);
  delay(100);
  beep(500, 1000);
  delay(100);
  beep(500, 1000);
}
