#include "arduino_secrets.h"
const char* ssid = "USU-guest";
const char* password = 0;

const char* MQTT_BROKER = "144.39.67.171";

const long interval = 10000;
unsigned long previousMillis = 0;

#include <WiFiS3.h>
#include <HydroServerMQTTClient.h>
WiFiClient wifiClient;
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);

Observation temperature = { "temperature", "uuidtemperature","tempsensorid"};
Observation ph = {"pH","uuidPh","phsensorid"};

Observation *observations[] = { &temperature, &ph };

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
  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);

  connectWiFi();
  Serial.println("connecting to broker");

  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("Arduinopublisher");

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

    float randomTemp,randomPh;
    randomTemp = random(200, 351) / 10.0;
    randomPh = random(1,7);
    previousMillis = currentMillis;
      
    temperature.value = randomTemp;
    ph.value = randomPh;

    uint8_t size = sizeof(observations)/sizeof(observations[0]);
    mqttClient.publishAll(observations,size,"2026-06-15T00:00:00Z");
    Serial.println(temperature.value);
    Serial.println(ph.value);
    };
  delay(10000);
}