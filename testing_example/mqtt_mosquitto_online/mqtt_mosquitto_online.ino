#include <WiFiS3.h>
#include <HydroServerMQTTClient.h>
// Create arduino_secrets.h in this sketch folder with: #define WIFI_SSID "..." and #define WIFI_PASS "..."
#include "arduino_secrets.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

// Using this, be very distinctive about the topic, client id so that you wouldn't get others message
// Any broker running over TCP port 1883
const char* MQTT_BROKER   = "54.36.178.49"; 
// const char* MQTT_BROKER   = "broker.hivemq.com"; 
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

void onMqttMessage(int messageSize) {
    String topic   = mqttClient.messageTopic(); 
    String payload = mqttClient.readMessage(); 

    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.println(payload);
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("running code for debug");
  connectWiFi();
  Serial.println("connecting to broker");

  // The callback in this will tell the device to do what message is received from subscribed topic
  mqttClient.onMessage(onMqttMessage);

  if(!mqttClient.connectToBroker()){
    Serial.println("connection not successful.Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    while (1);
  };
  Serial.println("connection successful");
  mqttClient.subscribe("rainfall");
}

void loop() {
  // poll is necessary so as to fire the callback
  mqttClient.poll();
  Serial.println("listening message");
  mqttClient.publishData("lro/test", "test from mayfly");
  // // publish every 5 seconds
  delay(5000);  
}
