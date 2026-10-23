// #include <Arduino.h>

// #include <HydroServerMQTTClient.h>
// #include <Sodaq_DS3231.h>
// #include <modems/ESP32Bee.h>

// #define SerialMon Serial
// #define SerialAT  Serial1
// const char* WIFI_SSID = "USU-guest";
// const char* WIFI_PASS = 0;

// const int8_t beePowerPin = 18;
// const long   modemBaud   = 38400;

// ESP32Bee modem(SerialAT, beePowerPin);
// // ESP32Bee modem(SerialAT,beePowerPin,SerialMon);

// void setup() {
//   SerialMon.begin(115200);
//   SerialAT.begin(modemBaud);    

//   SerialMon.println(F("Waking the modem.."));
//   modem.powerUp();
//   delay(3000);
//   if (modem.connectToInternet(WIFI_SSID, WIFI_PASS)) {
//     Serial.println(F("Wi-Fi connected"));
//     Serial.println(modem.getSignalQuality());
//     Serial.println(modem.getLocalIP());
//     Serial.println(modem.getFirmwareVersion());
//     Serial.println(modem.getModemInfo());
//   } else {
//     Serial.println(F("Wi-Fi connection failed, will retry each interval"));
//   }
// }

// void loop(){
//   Serial.println(modem.getSignalQuality());
//   delay(10000);
// }


#include <Arduino.h>
#include <HydroServerMQTTClient.h>
#include <modems/ESP32Bee.h>
#include <Sodaq_DS3231.h>

const char *SITE_CODE = "uwrl";
const char *CLIENT_ID = "esp32-02";
const char *MQTT_BROKER = "144.39.174.90";


// ---- Hardware ------------------------------------------------------------
HardwareSerial &beeSerial = Serial1; // Bee socket serial port
const int8_t BEE_POWER_PIN = 18;     // Bee socket power pin on Mayfly v1.x
const uint32_t BEE_BAUD = 57600;     // ESP32 AT firmware baud rate

ESP32Bee modem(beeSerial, BEE_POWER_PIN);
// To echo all AT commands to the Serial Monitor while debugging, use:
// ESP32Bee modem(beeSerial, BEE_POWER_PIN, Serial);

// createClient() allocates a new client, so call it once, here.
HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

Observation wifiSignal = {"wifiSignal", "signal",
                           "espressif32"};

uint32_t lastPublishedEpoch = 0;
const char* WIFI_SSID = "USU-guest";

const int8_t beePowerPin = 18;
const long   modemBaud   = 38400;

// ---- Helpers -------------------------------------------------------------
// Formats a DS3231 time as ISO 8601 UTC, e.g. 2026-09-23T18:30:00Z.
const char *getISO8601Time(DateTime &now) {
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(),
           now.month(), now.date(), now.hour(), now.minute(), now.second());
  return buf;
}

// ---- Sketch --------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  beeSerial.begin(modemBaud);

  modem.powerUp();
  delay(1000); // give the ESP32 time to boot

  Serial.println("Connecting to Wi-Fi");
  if (!modem.connectToInternet(WIFI_SSID, 0)) {
    Serial.println("Wi-Fi connection failed");
    while (true) {
    }
  }
  Serial.println("Wi-Fi connected");

  // getNISTTime() returns seconds since 2000-01-01, the DS3231's epoch
  rtc.begin();
  rtc.setDateTime(modem.getNISTTime());
  DateTime now = rtc.now();
  Serial.print("Clock set to ");
  Serial.println(getISO8601Time(now));

  // Required on the ESP32 before opening an MQTT connection
  modem.extraSetupForMQTT();

  mqttClient.setSiteCode(SITE_CODE);
  mqttClient.setClientID(CLIENT_ID);
  mqttClient.setKeepAliveInterval(100); // seconds

  Serial.print("Connecting to MQTT broker ");
  Serial.println(MQTT_BROKER);
  if (!mqttClient.connectToBroker()) {
    Serial.print("MQTT connection failed, error code: ");
    Serial.println(mqttClient.getConnectionError());
    while (true) {
    }
  }
  Serial.println("Connected to MQTT broker");
}

void loop() {
  mqttClient.poll();

  DateTime now = rtc.now();
  uint32_t epoch = now.getEpoch();

  // True only on the clock boundary (e.g. second :00 of every interval)
  bool onInterval = (epoch % 20 == 0);

  // The boundary lasts a whole second; publish only once per boundary
  if (onInterval && epoch != lastPublishedEpoch) {
    lastPublishedEpoch = epoch;

    // Simulated reading: replace with your own sensor code
    wifiSignal.value = modem.getSignalQuality();

    if (mqttClient.publishObservation(wifiSignal, getISO8601Time(now))) {
      Serial.print("Published to ");
      Serial.println(mqttClient.getObservationTopic(wifiSignal));
    } else {
      Serial.println("Publish failed (not connected?)");
    }
  }
}
