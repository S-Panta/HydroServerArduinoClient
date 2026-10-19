// ============================================================
// mqtt_basic.ino
//
// Arduino Uno R4 WiFi: connect to Wi-Fi, set the on-board RTC from NTP,
// connect to an MQTT broker and publish two simulated observations
// (temperature and pH) every 30 seconds.
//
// Hardware:  Arduino Uno R4 WiFi
// Libraries: HydroServerArduinoClient, ArduinoMqttClient, ArduinoJson,
//            NTPClient
//
// Before uploading:
//   1. Create arduino_secrets.h next to this sketch with
//        #define WIFI_SSID "your-network"
//        #define WIFI_PASS "your-password"
//   2. Set MQTT_BROKER, SITE_CODE and CLIENT_ID below.
//   3. Replace the datastream IDs with the UUIDs from your HydroServer
//      datastreams.
//
// Observations are published to: <siteCode>/<clientId>/<sensorId>/<property>
// e.g. uwrl/uno-r4-publisher/temp-sensor-1/temperature
// ============================================================

#include "arduino_secrets.h"
#include <HydroServerMQTTClient.h>
#include <NTPClient.h>
#include <RTC.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

// ---- Configuration -------------------------------------------------------
// Broker hostname or IP address. test.mosquitto.org is a public test broker;
// anyone can read what you publish there.
const char *MQTT_BROKER = "test.mosquitto.org";

// First two parts of every topic. Use your HydroServer site code and a
// short, unique, lowercase ID for this device.
const char *SITE_CODE = "uwrl";
const char *CLIENT_ID = "uno-r4-publisher";

// How often to publish (milliseconds)
const unsigned long PUBLISH_INTERVAL_MS = 30000;

// ---- Objects -------------------------------------------------------------
WiFiClient wifiClient;
// Only the network client and broker are required; the port defaults to 1883.
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Each Observation is one sensor reading:
//   observedProperty - what is measured (used in the topic)
//   datastreamId     - UUID of the HydroServer datastream (used in the payload)
//   sensorId         - your ID for the physical sensor (used in the topic)
Observation temperature = {"temperature", "YOUR-TEMPERATURE-DATASTREAM-UUID",
                           "temp-sensor-1"};
Observation ph = {"ph", "YOUR-PH-DATASTREAM-UUID", "ph-sensor-1"};

// Array of pointers, used by publishAll()
Observation *observations[] = {&temperature, &ph};
const uint8_t OBSERVATION_COUNT =
    sizeof(observations) / sizeof(observations[0]);

unsigned long lastPublishMs = 0;

// ---- Helpers -------------------------------------------------------------
// Returns the RTC time as an ISO 8601 UTC string, e.g. 2026-09-23T18:30:00Z.
// The buffer is static so the pointer stays valid after the function returns.
const char *getISO8601Time() {
  RTCTime t;
  RTC.getTime(t);
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t.getYear(),
           Month2int(t.getMonth()), t.getDayOfMonth(), t.getHour(),
           t.getMinutes(), t.getSeconds());
  return buf;
}

void connectWiFi() {
  Serial.print("Connecting to Wi-Fi ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // DHCP can take a few seconds to assign an address (0.0.0.0 until then)
  while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
    delay(500);
  }
  Serial.print("Wi-Fi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

// Sets the on-board RTC to UTC from NTP. Run after Wi-Fi is connected and
// before publishing anything.
void syncRTCFromNTP() {
  timeClient.begin();
  while (!timeClient.update()) {
    delay(500);
  }
  RTC.begin();
  RTCTime now(timeClient.getEpochTime());
  RTC.setTime(now);
  Serial.print("RTC set to ");
  Serial.println(getISO8601Time());
}

// ---- Sketch --------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000) {
  }

  connectWiFi();
  syncRTCFromNTP();

  // Site code and client ID build every topic; set them before connecting.
  mqttClient.setSiteCode(SITE_CODE);
  mqttClient.setClientID(CLIENT_ID);

  // Optional: message the broker publishes to <siteCode>/<clientId>/lwt if
  // this device drops off without disconnecting. Call after the two setters
  // above and before connecting.
  mqttClient.setLastWill("offline");

  // Optional: uncomment if your broker needs a username and password
  // mqttClient.setAuthentication("username", "password");

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
  // Keeps the connection alive; call on every pass through loop()
  mqttClient.poll();

  if (millis() - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = millis();

    // Simulated readings: replace with your own sensor code
    temperature.value = random(200, 351) / 10.0;
    ph.value = random(60, 85) / 10.0;

    const char *timestamp = getISO8601Time();

    // Publish one observation...
    mqttClient.publishObservation(temperature, timestamp);
    Serial.print("Published to ");
    Serial.println(mqttClient.getObservationTopic(temperature));

    // ...or several with the same timestamp
    // mqttClient.publishAll(observations, OBSERVATION_COUNT, timestamp);
    mqttClient.publishObservation(ph, timestamp);
    Serial.print("Published to ");
    Serial.println(mqttClient.getObservationTopic(ph));
  }
}
