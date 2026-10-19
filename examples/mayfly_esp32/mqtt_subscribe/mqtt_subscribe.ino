// ============================================================
// mqtt_subscribe.ino
//
// EnviroDIY Mayfly + ESP32 Wi-Fi Bee: publish a simulated temperature
// observation every 10 seconds AND print values received on two subscribed
// topics.
//
// Hardware:  EnviroDIY Mayfly v1.x, ESP32 Bee (Espressif AT firmware)
// Libraries: HydroServerArduinoClient, ArduinoMqttClient, ArduinoJson,
//            EnviroDIY TinyGSM fork, StreamDebugger, Sodaq_DS3231
//
// Before uploading:
//   1. Create arduino_secrets.h next to this sketch with
//        #define WIFI_SSID "your-network"
//        #define WIFI_PASS "your-password"   // use 0 for an open network
//   2. Set MQTT_BROKER, SITE_CODE, CLIENT_ID and the subscribed topics below.
//
// subscribe("a/b/c") subscribes to <siteCode>/a/b/c. Incoming messages are
// expected to be observation JSON payloads, like the ones this library sends.
// ============================================================

#include "arduino_secrets.h"
#include <HydroServerMQTTClient.h>
#include <Sodaq_DS3231.h>
#include <modems/EspressifESP32.h>

// ---- Configuration -------------------------------------------------------
const char *MQTT_BROKER = "test.mosquitto.org";
const char *SITE_CODE = "uwrl";
const char *CLIENT_ID = "mayfly-esp32-subscriber";

// Topics to listen to, relative to the site code
const char *TEMPERATURE_TOPIC = "python_publisher/sensorid/temperature_celsius";
const char *PRESSURE_TOPIC = "python_publisher/sensorid/pressure_hpa";

const unsigned long PUBLISH_INTERVAL_MS = 10000;

// ---- Hardware ------------------------------------------------------------
HardwareSerial &beeSerial = Serial1; // Bee socket serial port
const int8_t BEE_POWER_PIN = 18;     // Bee socket power pin on Mayfly v1.x
const uint32_t BEE_BAUD = 57600;     // ESP32 AT firmware baud rate

EspressifESP32 modem(beeSerial, BEE_POWER_PIN);
// EspressifESP32 modem(beeSerial, BEE_POWER_PIN, Serial); // echo AT traffic

HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

Observation temperature = {"temperature", "YOUR-TEMPERATURE-DATASTREAM-UUID",
                           "temp-sensor-1"};

unsigned long lastPublishMs = 0;

// ---- Helpers -------------------------------------------------------------
const char *getISO8601Time() {
  DateTime now = rtc.now();
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(),
           now.month(), now.date(), now.hour(), now.minute(), now.second());
  return buf;
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

  rtc.begin();
  rtc.setDateTime(modem.getNISTTime());
  Serial.print("Clock set to ");
  Serial.println(getISO8601Time());

  // Required on the ESP32 before opening an MQTT connection
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

  mqttClient.onMessage(onMessageReceived);
  mqttClient.subscribe(TEMPERATURE_TOPIC);
  mqttClient.subscribe(PRESSURE_TOPIC);
}

void loop() {
  mqttClient.poll();

  if (millis() - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = millis();

    // Simulated reading: replace with your own sensor code
    temperature.value = random(200, 251) / 10.0;

    if (mqttClient.publishObservation(temperature, getISO8601Time())) {
      Serial.println("Observation published");
    } else {
      Serial.println("Publish failed (not connected?)");
    }
  }
}
