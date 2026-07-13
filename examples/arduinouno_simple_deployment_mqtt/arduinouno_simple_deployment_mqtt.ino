#include "arduino_secrets.h"
#include <HydroServerMQTTClient.h>
#include <WiFiS3.h>

const char *MQTT_BROKER = "raspberrypi1.mypc.usu.edu";
const char *datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";

const char *ssid = "USU-guest";
const char *password = 0;

const char *SITE_CODE = "uwrl";
WiFiClient wifiClient;
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);

Observation temperature = {"Temperature", "uuid-temperature"};

// for realtime clock
#include <Sodaq_DS3231.h>

void connectToWiFi() {
  delay(2000);
  Serial.print("Connecting to WiFi: ");
  WiFi.begin(ssid, password);

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

String getISO8601Timestamp() {
  DateTime now = rtc.now();
  char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(),
           now.month(), now.date(), now.hour(), now.minute(), now.second());
  return String(buf);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);

  connectToWiFi();

  mqttClient.setKeepAliveInterval(150000UL);
  mqttClient.setClientID("arduino-enlab");
  if (!mqttClient.connectToBroker()) {
    Serial.println("Cannot connect to Broker. Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    // it make no sense to work further when connection to broker is not
    // successful
    while (1)
      ;
  };

  Serial.println("Connection to Broker Successful");

  rtc.begin();
  rtc.setDateTime(DateTime(2026, 7, 7, 14, 30, 0, 2));
}

void loop() {
  mqttClient.poll();
  String currentTimeStamp = getISO8601Timestamp();
  temperature.value = 30;
  mqttClient.publishObservation(temperature, currentTimeStamp.c_str());
  delay(10000);
}
