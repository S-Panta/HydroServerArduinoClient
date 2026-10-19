// ============================================================
// mqtt_basic.ino
//
// EnviroDIY Mayfly + Digi XBee S6B Wi-Fi Bee: connect to Wi-Fi, set the
// Mayfly's DS3231 clock from NIST, connect to an MQTT broker and publish
// the air temperature from the Mayfly's on-board SHT4x sensor every
// 30 seconds.
//
// Hardware:  EnviroDIY Mayfly v1.x (on-board SHT4x), Digi XBee S6B Wi-Fi Bee
// Libraries: HydroServerArduinoClient, ArduinoMqttClient, ArduinoJson,
//            EnviroDIY TinyGSM fork, StreamDebugger, Sodaq_DS3231,
//            Adafruit SHT4x
//
// Before uploading:
//   1. Create arduino_secrets.h next to this sketch with
//        #define WIFI_SSID "your-network"
//        #define WIFI_PASS "your-password"   // use 0 for an open network
//   2. Set MQTT_BROKER, SITE_CODE and CLIENT_ID below.
//
// The XBee S6B has no TLS support in this library: use plain MQTT (1883).
// ============================================================

#include "arduino_secrets.h"
#include <Adafruit_SHT4x.h>
#include <HydroServerMQTTClient.h>
#include <Sodaq_DS3231.h>
#include <modems/DigiXbeeS6B.h>

// ---- Configuration -------------------------------------------------------
// Prefer a hostname. If you use an IP address, the XBee's destination
// address (DL) may also need updating in XCTU.
const char *MQTT_BROKER = "test.mosquitto.org";
const char *SITE_CODE = "uwrl";
const char *CLIENT_ID = "mayfly-xbee-01";

const unsigned long PUBLISH_INTERVAL_MS = 30000;

// ---- Hardware ------------------------------------------------------------
HardwareSerial &beeSerial = Serial1; // Bee socket serial port
const int8_t BEE_POWER_PIN = 18;     // Bee socket power pin on Mayfly v1.x
const uint32_t BEE_BAUD = 9600;      // XBee S6B default baud rate

DigiXbeeS6B modem(beeSerial, BEE_POWER_PIN);
// To echo all AT commands to the Serial Monitor while debugging, use:
// DigiXbeeS6B modem(beeSerial, BEE_POWER_PIN, Serial);

// createClient() allocates a new client, so call it once, here.
HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

Adafruit_SHT4x sht4;

Observation airTemperature = {"air_temperature",
                              "YOUR-AIR-TEMPERATURE-DATASTREAM-UUID", "sht4x"};

unsigned long lastPublishMs = 0;

// ---- Helpers -------------------------------------------------------------
const char *getISO8601Time() {
  DateTime now = rtc.now();
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(),
           now.month(), now.date(), now.hour(), now.minute(), now.second());
  return buf;
}

// ---- Sketch --------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  beeSerial.begin(BEE_BAUD);

  modem.powerUp();
  delay(2000); // give the XBee time to boot

  Serial.println("Connecting to Wi-Fi");
  if (!modem.connectToInternet(WIFI_SSID, WIFI_PASS)) {
    Serial.println("Wi-Fi connection failed");
    while (true) {
    }
  }
  Serial.println("Wi-Fi connected");

  // getNISTTime() returns seconds since 2000-01-01 (the DS3231's epoch),
  // or 0 if the time server could not be reached
  rtc.begin();
  uint32_t nistTime = modem.getNISTTime();
  if (nistTime != 0) {
    rtc.setDateTime(nistTime);
  } else {
    Serial.println("Could not reach the time server; clock not set");
  }
  Serial.print("Clock reads ");
  Serial.println(getISO8601Time());

  if (!sht4.begin()) {
    Serial.println("SHT4x sensor not found");
    while (true) {
    }
  }

  // Does nothing on the XBee, but keeps the sketch the same shape as the
  // ESP32 version
  modem.extraSetupForMQTT();

  mqttClient.setSiteCode(SITE_CODE);
  mqttClient.setClientID(CLIENT_ID);

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

  if (millis() - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = millis();

    sensors_event_t humidity, temperature;
    sht4.getEvent(&humidity, &temperature);
    airTemperature.value = temperature.temperature;

    if (mqttClient.publishObservation(airTemperature, getISO8601Time())) {
      Serial.print("Published to ");
      Serial.println(mqttClient.getObservationTopic(airTemperature));
    } else {
      Serial.println("Publish failed (not connected?)");
    }
  }
}
