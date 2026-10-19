// ============================================================
// mqtt_basic.ino
//
// EnviroDIY Mayfly + ESP32 Wi-Fi Bee: connect to Wi-Fi, set the Mayfly's
// DS3231 clock from an NTP server, connect to an MQTT broker and publish a
// simulated temperature observation at the start of every minute.
//
// Hardware:  EnviroDIY Mayfly v1.x, ESP32 Bee (Espressif AT firmware)
// Libraries: HydroServerArduinoClient, ArduinoMqttClient, ArduinoJson,
//            EnviroDIY TinyGSM fork, StreamDebugger, Sodaq_DS3231
//
// Before uploading:
//   1. Create arduino_secrets.h next to this sketch with
//        #define WIFI_SSID "your-network"
//        #define WIFI_PASS "your-password"   // use 0 for an open network
//   2. Set MQTT_BROKER, SITE_CODE and CLIENT_ID below.
// ============================================================

#include "arduino_secrets.h"
#include <HydroServerMQTTClient.h>
#include <Sodaq_DS3231.h>
#include <modems/EspressifESP32.h>

// ---- Configuration -------------------------------------------------------
const char *MQTT_BROKER = "test.mosquitto.org";
const char *SITE_CODE = "uwrl";
const char *CLIENT_ID = "mayfly-esp32-01";

// Publish when the clock reaches a multiple of this many minutes
const uint32_t PUBLISH_INTERVAL_MINUTES = 1;

// ---- Hardware ------------------------------------------------------------
HardwareSerial &beeSerial = Serial1; // Bee socket serial port
const int8_t BEE_POWER_PIN = 18;     // Bee socket power pin on Mayfly v1.x
const uint32_t BEE_BAUD = 57600;     // ESP32 AT firmware baud rate

EspressifESP32 modem(beeSerial, BEE_POWER_PIN);
// To echo all AT commands to the Serial Monitor while debugging, use:
// EspressifESP32 modem(beeSerial, BEE_POWER_PIN, Serial);

// createClient() allocates a new client, so call it once, here.
HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

Observation temperature = {"temperature", "YOUR-TEMPERATURE-DATASTREAM-UUID",
                           "temp-sensor-1"};

uint32_t lastPublishedEpoch = 0;

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
  beeSerial.begin(BEE_BAUD);

  modem.powerUp();
  delay(1000); // give the ESP32 time to boot

  Serial.println("Connecting to Wi-Fi");
  if (!modem.connectToInternet(WIFI_SSID, WIFI_PASS)) {
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
  bool onInterval = (epoch % (PUBLISH_INTERVAL_MINUTES * 60) == 0);

  // The boundary lasts a whole second; publish only once per boundary
  if (onInterval && epoch != lastPublishedEpoch) {
    lastPublishedEpoch = epoch;

    // Simulated reading: replace with your own sensor code
    temperature.value = random(200, 251) / 10.0;

    if (mqttClient.publishObservation(temperature, getISO8601Time(now))) {
      Serial.print("Published to ");
      Serial.println(mqttClient.getObservationTopic(temperature));
    } else {
      Serial.println("Publish failed (not connected?)");
    }
  }
}
