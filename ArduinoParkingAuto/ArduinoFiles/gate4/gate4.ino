#include <SPI.h>
#include <MFRC522.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

// RFID
#define SS_PIN 5
#define RST_PIN 4
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Firebase
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#define WIFI_SSID "lippyphone"
#define WIFI_PASSWORD "garamlaut"
#define API_KEY "AIzaSyB6EvUkC7g8MAUdbQG5nkB1MPY0lT4sdbE"
#define DATABASE_URL "arduinoparkingauto-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "testemail@gmail.com"
#define USER_PASSWORD "123123"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;

void setup() { 
  Serial.begin(115200);

  //RFID
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  delay(3000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 25);
  // Display the text passed as a parameter
  display.println("Scan card!");
  display.display(); 

  // Firebase
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
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

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  Firebase.begin(&config, &auth);

  Firebase.setDoubleDigits(5);
}
 
void loop() {
  if ( ! rfid.PICC_IsNewCardPresent())
    return;
  if ( ! rfid.PICC_ReadCardSerial())
    return;
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  // Current scanned RFID
  byte scannedRFID[] = {rfid.uid.uidByte[0], rfid.uid.uidByte[1], rfid.uid.uidByte[2], rfid.uid.uidByte[3]};
  char scannedRFIDString[9];
  sprintf(scannedRFIDString, "%02X%02X%02X%02X", scannedRFID[0], scannedRFID[1], scannedRFID[2], scannedRFID[3]);
  Serial.printf("Current RFID : %s\n", scannedRFIDString);

  // Check empty
  if (strcmp(Firebase.getString(fbdo, F("/slot1/card")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str(), "empty") == 0
  && strcmp(Firebase.getString(fbdo, F("/slot2/card")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str(), scannedRFIDString) != 0){
    // update state & card
    Serial.printf("Set state 1... %s\n", Firebase.setString(fbdo, F("/slot1/state"), "PARK_CAR") ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set card 1... %s\n", Firebase.setString(fbdo, F("/slot1/card"), scannedRFIDString) ? "ok" : fbdo.errorReason().c_str());
    oled("Park at Slot 1");
  } else if (strcmp(Firebase.getString(fbdo, F("/slot2/card")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str(), "empty") == 0
  && strcmp(Firebase.getString(fbdo, F("/slot1/card")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str(), scannedRFIDString) != 0){
    // update state & card
      Serial.printf("Set state 2... %s\n", Firebase.setString(fbdo, F("/slot2/state"), "PARK_CAR") ? "ok" : fbdo.errorReason().c_str());
      Serial.printf("Set card 2... %s\n", Firebase.setString(fbdo, F("/slot2/card"), scannedRFIDString) ? "ok" : fbdo.errorReason().c_str());
      oled("Park at Slot 2");
  } else if (strcmp(Firebase.getString(fbdo, F("/slot1/card")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str(), scannedRFIDString) == 0
  || strcmp(Firebase.getString(fbdo, F("/slot2/card")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str(), scannedRFIDString) == 0){
      oled("Card already used");
  } else {
    oled("Parking lot full");
  }

 	// Halt PICC
 	rfid.PICC_HaltA();
 	// Stop encryption on PCD
 	rfid.PCD_StopCrypto1();
  delay(3000);
  oled("Scan card!");
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(' ');
    Serial.print(buffer[i], DEC);
  }
}

void oled(const char* string) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 25);
  display.println(string);
  display.display(); 
}

