#include <WiFiS3.h>
#include <HydroServerMQTTClient.h>
// Create arduino_secrets.h in this sketch folder with: #define WIFI_SSID "..." and #define WIFI_PASS "..."
#include "arduino_secrets.h"
#include "mqtt_secrets.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

Observation streamTemperature;
// Using this, be very distinctive about the topic, client id so that you wouldn't get others message
// Any broker running over TCP port 1883
const char* MQTT_BROKER   = "mqtt3.thingspeak.com"; 
 
const int   MQTT_PORT     = 1883;

WiFiClient wifiClient;

HydroServerMQTTClient mqttClient(wifiClient,MQTT_BROKER);

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  WiFi.begin(ssid,password);
  Serial.println(WiFi.status());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Cannot connected to wifi");
    while(1);
  }
  delay(5000);
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("running code for debug");
  connectWiFi();
  Serial.println("connecting to broker");
  mqttClient.setClientID(SECRET_MQTT_CLIENT_ID);
  mqttClient.setAuthentication(SECRET_MQTT_PASSWORD,SECRET_MQTT_PASSWORD);

  if(!mqttClient.connectToBroker()){
    Serial.println("connection not successful.Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    while (1);
  };
  Serial.println("connection successful");
  streamTemperature.observedProperty = "Temperature";
  streamTemperature.datastreamId = "uuid-temperature";

  mqttClient.setKeepAliveInterval(150000UL);
  mqttClient.setClientID("mayfly-enlab");
  mqttClient.setSiteCode("urwl");
}

void loop() {
  // poll is necessary so as to fire the callback
  mqttClient.poll();
  Serial.println("listening message");
  streamTemperature.value = 123
   mqttClient.publishObservation(
    streamTemperature,
    "2026-06-15T00:00:00Z"
  );
  // // publish every 5 seconds
  delay(5000);  
}
