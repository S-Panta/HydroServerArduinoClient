// ============================================================
// mqtt_subscribe.ino
//
// Arduino Uno R4 WiFi: publish two simulated observations (temperature and
// pH) every 30 seconds AND print values received on two subscribed topics.
//
// Hardware:  Arduino Uno R4 WiFi
// Libraries: HydroServerArduinoClient, ArduinoMqttClient, ArduinoJson,
//            NTPClient
//
// Before uploading:
//   1. Create arduino_secrets.h next to this sketch with
//        #define WIFI_SSID "your-network"
//        #define WIFI_PASS "your-password"
//   2. Set MQTT_BROKER, SITE_CODE, CLIENT_ID and the subscribed topics below.
//
// subscribe("a/b/c") subscribes to <siteCode>/a/b/c. Incoming messages are
// expected to be observation JSON payloads, like the ones this library sends.
// ============================================================

#include "arduino_secrets.h"
#include <HydroServerMQTTClient.h>
#include <NTPClient.h>
#include <RTC.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

// ---- Configuration -------------------------------------------------------
const char *MQTT_BROKER = "test.mosquitto.org";
const char *SITE_CODE = "uwrl";
const char *CLIENT_ID = "uno-r4-subscriber";

// Topics to listen to, relative to the site code
const char *TEMPERATURE_TOPIC = "python_publisher/sensorid/temperature_celsius";
const char *PRESSURE_TOPIC = "python_publisher/sensorid/pressure_hpa";

const unsigned long PUBLISH_INTERVAL_MS = 30000;

// ---- Objects -------------------------------------------------------------
WiFiClient wifiClient;
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

Observation temperature = {"temperature", "YOUR-TEMPERATURE-DATASTREAM-UUID",
                           "temp-sensor-1"};
Observation ph = {"ph", "YOUR-PH-DATASTREAM-UUID", "ph-sensor-1"};

Observation *observations[] = {&temperature, &ph};
const uint8_t OBSERVATION_COUNT =
    sizeof(observations) / sizeof(observations[0]);

// millis()-based timing instead of delay() so poll() keeps running and
// incoming messages are handled promptly.
unsigned long lastPublishMs = 0;

// ---- Helpers -------------------------------------------------------------
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
  while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
    delay(500);
  }
  Serial.print("Wi-Fi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

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

// Runs inside poll() each time a message arrives on a subscribed topic.
// The payload can only be read once: parse it with deserializePayload()
// (or read it raw with readMessage()), then look up the value by topic.
void onMessageReceived(int messageSize) {
  String topic = mqttClient.messageTopic();
  mqttClient.deserializePayload();

  Serial.print("Received on ");
  Serial.print(topic);
  Serial.print(": ");
  if (topic.endsWith(TEMPERATURE_TOPIC)) {
    Serial.println(mqttClient.getLatestValueofTopic(TEMPERATURE_TOPIC));
  } else if (topic.endsWith(PRESSURE_TOPIC)) {
    Serial.println(mqttClient.getLatestValueofTopic(PRESSURE_TOPIC));
  }
}

// ---- Sketch --------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000) {
  }

  connectWiFi();
  syncRTCFromNTP();

  mqttClient.setSiteCode(SITE_CODE);
  mqttClient.setClientID(CLIENT_ID);
  // Optional:
  // mqttClient.setLastWill("offline");
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

  // Register the callback, then subscribe (must be connected first)
  mqttClient.onMessage(onMessageReceived);
  mqttClient.subscribe(TEMPERATURE_TOPIC);
  mqttClient.subscribe(PRESSURE_TOPIC);
}

void loop() {
  // Sends keep-alive pings and delivers incoming messages to the callback
  mqttClient.poll();

  if (millis() - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = millis();

    // Simulated readings: replace with your own sensor code
    temperature.value = random(200, 351) / 10.0;
    ph.value = random(60, 85) / 10.0;

    uint8_t published = mqttClient.publishAll(observations, OBSERVATION_COUNT,
                                              getISO8601Time());
    Serial.print("Published ");
    Serial.print(published);
    Serial.print(" of ");
    Serial.print(OBSERVATION_COUNT);
    Serial.println(" observations");
  }
}
