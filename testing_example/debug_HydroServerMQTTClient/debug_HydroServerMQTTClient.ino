#include "arduino_secrets.h"
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;


const char* MQTT_BROKER = "192.168.0.101";
const int MQTT_PORT = 1883;



const long interval = 10000;
unsigned long previousMillis = 0;

const char* SITE_CODE = "uwrl";

#include <WiFiS3.h>
#include <HydroServerMQTTClient.h>
WiFiClient wifiClient;
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);

const char* datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";
// DataStream phDatastream   = { "pH",          "uuid-ph",         };

Observation temperature = { "temperature", "uuidtemperature","tempsensor"};
// DataStream* datastreams[] = { &tempDatastream, &phDatastream };

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


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("running sketch mqtt_basic.ino");
  connectWiFi();
  
  Serial.println("connecting to broker");
  
  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("Arduinopublisher");
  mqttClient.setLastWill("Shutting down the arduion");

  if(!mqttClient.connectToBroker()){
    Serial.println("connection not successful.Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    while (1);
  };

  Serial.println("connection successful");
}

void loop() {
 
  mqttClient.poll();
 
  Serial.print("loop runs  every 10 seconds");
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    Serial.println("publishing every 10 second");

    float randomTemp;
    randomTemp = random(200, 351) / 10.0;
    previousMillis = currentMillis;
      
    temperature.value = randomTemp;
      // phDatastream.value   = 7.2;
    Serial.println(mqttClient.publishObservation(temperature,"2026-06-15T00:00:00Z"));
  };
 delay(10000);

  //   // mqttClient.publishAll(datastreams, 2,  "2026-06-15T00:00:00Z");
  //   // mqttClient.publishObservation(
  //   //   datastreamId, 
  //   //   25.5,
  //   //   "2026-06-15T00:00:00Z"
  //   // );
  //   Serial.println("delay for 5 second");
    // // if retain = true
    // // broker logs changes
    // // publish every 5 seconds
  // }
}