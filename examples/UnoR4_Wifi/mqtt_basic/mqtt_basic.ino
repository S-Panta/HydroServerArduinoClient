// ============================================================
// UnoR4_MQTT_Basic.ino
//
// Basic example: connect an Arduino Uno R4 WiFi board to a WiFi
// network, connect to a MQTT broker, and publish
// simulated sensor observations (temperature, pH)
//
// This is meant as a minimal starting point — swap the random
// values below for real sensor readings in your own project.
// ============================================================

// Wifi credentials
// if you want to add wifi ssid and password from arduino_secrets
// arduino_secrets.h should be in same level as this sketch
#include "arduino_secrets.h"

const char *ssid = "USU-guest";
const char *password = 0;

// MQTT broker details
// You can use either url or ip address of mqtt broker.
// const char* MQTT_BROKER = "144.39.67.171";
// const char *MQTT_BROKER = "test.mosquitto.org";
// const char *MQTT_BROKER = "raspberrypi1.mypc.usu.edu";
const char *MQTT_BROKER = "144.39.51.28";

// data publish interval
const long interval = 30000;
unsigned long previousMillis = 0;

// importing wifi driver for Arduino Uno R4
#include <WiFiS3.h>

#include <HydroServerMQTTClient.h>
WiFiClient wifiClient;

// Create the HydroServer MQTT client. 
// wificlient and broker address are necessary while other are optional
// By default, the broker port is 1883.
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);

// Observations to publish
// Each Observation represents one sensor reading,
//   1. observedProperty — human-readable name of what's measured
//   2. datastreamId     — UUID identifying the target datastream on HydroServer
//   3. sensorId         — identifier for the physical sensor
Observation temperature = {"temperature", "uuidtemperature", "tempsensorid"};
Observation ph = {"pH", "uuidPh", "phsensorid"};

// Array of POINTERS to the observations above.
Observation *observations[] = {&temperature, &ph};

// This is for realtime clock
#include "RTC.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org");

// Reads the RTC and convert into iso time
char *getISO8601Time() {
  RTCTime t;
  RTC.getTime(t);
  // static: buffer must outlive the function return
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t.getYear(),
           Month2int(t.getMonth()), t.getDayOfMonth(), t.getHour(),
           t.getMinutes(), t.getSeconds());
  return buf;
}

void connectWiFi() {
  delay(2000);
  Serial.print("Connecting to WiFi: ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    while (1)
      ;
  }
  Serial.println("\nWiFi connected!");
  // some time requires for DHCP to assign IP
  delay(5000);
  // IP address if not 0.0.0.0 means the device is connected to wifi
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Must run after Wi-Fi is connected and before any publish
void syncRTCFromNTP() {
  timeClient.update();
  unsigned long unixTime = timeClient.getEpochTime();
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);
  Serial.print("RTC set to: ");
  Serial.println(timeClient.getFormattedTime());
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);

  connectWiFi();

  // setting time from server
  timeClient.update();

  RTC.begin();
  syncRTCFromNTP();
  Serial.println("connecting to broker");

  // sitecode and client id are necessary setters. They are used in generating
  // topic for publishing observations observation is published as
  // siteCode/clientId/sensorId/observedProperty
  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("Arduinopublisher");
  // The broker will publish last will message to the subscriber if this
  // publisher device shut down
  mqttClient.setLastWill("arduino uno is shutting down", true, 2);
  Serial.println(mqttClient.getLastWillTopic());

  // if your broker requires a username/password
  // mqttClient.setAuthentication("myUsername", "myPassword");
  if (!mqttClient.connectToBroker()) {
    Serial.println("Connection to Broker failed. Connection Error is");
    Serial.println(mqttClient.getConnectionError());
    while (1)
      ;
  };
  Serial.println("connection to mqtt broker successful");
}

void loop() {

  mqttClient.poll();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    Serial.println("publishing every 30 seconds");
    // Simulated sensor readings.
    float randomTemp, randomPh;
    randomTemp = random(200, 351) / 10.0;
    randomPh = random(1, 7);
    previousMillis = currentMillis;

    temperature.value = randomTemp;
    ph.value = randomPh;
    mqttClient.publishObservation(temperature, getISO8601Time());
    mqttClient.publishObservation(ph, getISO8601Time());
    Serial.println(mqttClient.getObservationTopic(temperature));

    // For publishing multiple observations at the same time
    // uint8_t size = sizeof(observations) / sizeof(observations[0]);
    // mqttClient.publishAll(observations, size, getISO8601Time());
  };
}