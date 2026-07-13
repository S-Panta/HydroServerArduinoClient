#include <WiFiS3.h>
#include <HydroServerMQTTClient.h>
#include "arduino_secrets.h"


const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;


const char* MQTT_BROKER = "192.168.0.101";
const int MQTT_PORT = 1883;

const char* datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";

WiFiClient wifiClient;
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);

void connectWiFi() {
  delay(2000);
  Serial.print("Connecting to WiFi: ");
  WiFi.begin(ssid,password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  // some time requires for DHCP to assign IP
  delay(5000);
  // IP address if not 0.0.0.0 means the device is connected to wifi
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
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
  Serial.println("running sketch mqtt_basic.ino");
  connectWiFi();
  
  Serial.println("connecting to broker");

  mqttClient.setClientID("mayfly data logger");
  mqttClient.setLastWill("arduino/status", "device failed");
  mqttClient.setKeepAliveInterval(98);
  // The callback in this will tell the device to do what message is received from subscribed topic
  mqttClient.onMessage(onMqttMessage);

  if(!mqttClient.connectToBroker()){
    Serial.println("connection not successful.Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    while (1);
  };

  Serial.println("connection successful");
  // mqttClient.subscribe("lro/greeting");
}

void loop() {
  // poll is necessary so as to fire the callback
  mqttClient.poll();
  mqttClient.publishObservation(
    datastreamId, 
    25.5,
    "2026-06-15T00:00:00Z"
  );
  // if retain = true
  // broker logs changes
  // publish every 5 seconds
  delay(5000);
}