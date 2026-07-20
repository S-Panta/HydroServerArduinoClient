// ============================================================
// UnoR4_MQTT_Basic.ino
//
// Basic example: connect an Arduino Uno R4 WiFi board to a WiFi
// network, connect to a MQTT broker, and publish
// simulated sensor observations (temperature, pH) every 10 seconds.
//
// This is meant as a minimal starting point — swap the random
// values below for real sensor readings in your own project.
// ============================================================

// Wifi credentials
const char *ssid = "USU-guest";
const char *password =
    0; // NULL password — this is an open network (no password required)

// MQTT broker details
// You can use both url or ip address of mqtt broker.

// const char* MQTT_BROKER = "144.39.67.171";
const char *MQTT_BROKER = "test.mosquitto.org";

// publish interval; here 30 seconds
const long interval = 30000;
unsigned long previousMillis = 0;

// this is importing wifi driver specific to the Arduino Uno R4 board
#include <WiFiS3.h>

#include <HydroServerMQTTClient.h>
WiFiClient wifiClient;

// Create the HydroServer MQTT client.
// wificlient and broker address are necessary while other are optional
// parameter by default, the broker port is 1883, so it is not necessary to pass
// unless otherwise.
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);

// Observations to publish
// Each Observation represents one sensor reading,
//   1. observedProperty — human-readable name of what's measured
//   2. datastreamId     — UUID identifying the target datastream on HydroServer
//   3. sensorId         — identifier for the physical sensor
//   4. value            — the actual reading (defaults to 0.0, set later in
//   loop())

Observation temperature = {"temperature", "uuidtemperature", "tempsensorid"};
Observation ph = {"pH", "uuidPh", "phsensorid"};

// Array of POINTERS to the observations above.
Observation *observations[] = {&temperature, &ph};

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

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);

  connectWiFi();
  Serial.println("connecting to broker");

  // sitecode and client id are necessary setters. They are used in generating
  // topic for publishing observations observation is published as
  // <sitecode>/<clientId>/<sensorId>/<observedProperty>/observations
  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("Arduinopublisher");
  // optional mqtt parameters
  // Optional: message the broker will publish if this device disconnects
  // unexpectedly mqttClient.setLastWill("arduino uno is shutting down");

  // Optional: authenticate if your broker requires a username/password
  // mqttClient.setAuthentication("myUsername", "myPassword");

  if (!mqttClient.connectToBroker()) {
    Serial.println("Connection to Broker failed. Connection Error is");
    Serial.println(mqttClient.getConnectionError());
    while (1)
      ;
  };
  Serial.println("connection successful");
}

void loop() {

  // Must be called regularly to keep the MQTT connection alive — handles
  // PINGREQ/PINGRESP with the broker plus any incoming subscribed messages.
  // The broker disconnects a client if it receives nothing for roughly
  // 1.5x the keep-alive interval (default is 90 for the library).
  // If requires, adjust the interval with setKeepAliveInterval() before
  // connecting.

  mqttClient.poll();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    Serial.println("publishing every 30 seconds");

    // Simulated sensor readings. This should be replaced with the real sensor
    // measurement.
    float randomTemp, randomPh;
    randomTemp = random(200, 351) / 10.0;
    randomPh = random(1, 7);
    previousMillis = currentMillis;

    temperature.value = randomTemp;
    ph.value = randomPh;
    // To publish just one observation, use publishObservation() instead:
    // mqttClient.publishObservation(temperature,"2026-06-15T00:00:00Z");

    // For publishing multiple observation at the same time
    uint8_t size = sizeof(observations) / sizeof(observations[0]);
    mqttClient.publishAll(observations, size, "2026-06-15T00:00:00Z");
  };
  delay(10000);
}