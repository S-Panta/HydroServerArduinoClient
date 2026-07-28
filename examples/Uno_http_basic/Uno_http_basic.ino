// for arduino uno
#include "arduino_secrets.h"
#include <HydroServerHTTPClient.h>
#include <WiFiS3.h>

const char *ssid = "USU-guest";
const char *password = 0;

unsigned long lastPostTime = 0;
const unsigned long postInterval = 20000;
int currentHour = 3;

// // for playground
const char *apiKey = PLAYGROUND_API_KEY;
const char *datastreamId = "019f246b-c5b9-7b45-aac6-261adc526b55";
const char *serverAddress = "playground.hydroserver.org";
const int serverPort = 443;

// for local
// const char *serverAddress = "144.39.67.171";
// const int serverPort = 80;
// const char *apiKey = LOCAL_API_KEY;
// const char *datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";

// for playground instance
WiFiClient wifiClient;
WiFiSSLClient sslClient;
HydroServerHTTPClient hsClient(sslClient, serverAddress, serverPort);

// for local instance
// WiFiClient wifiClient;
// HydroServerHTTPClient hsClient(wifiClient, serverAddress, serverPort);

Observation temperature = {"Temperature", datastreamId};
const char *streamTempDataStreamId = "019fa988-631d-7dc1-886f-b80431339ff7";
Observation streamTemperature = {"StreamTemperature", streamTempDataStreamId};

Observation *observations[] = {&temperature, &streamTemperature};

// for arduino uno
void connectWiFi() {
  delay(2000);
  Serial.print("Connecting to WiFi: ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("....");
  }
  Serial.println("\nWiFi connected!");
  // some time requires for DHCP to assign IP
  delay(5000);
  // IP address if not 0.0.0.0 means the device is connected to wifi
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

String getNextTimestampISO8601() {
  char buffer[25];
  snprintf(buffer, sizeof(buffer), "2026-07-17T%02d:59:43Z", currentHour);
  currentHour = (currentHour + 1) % 24;
  return String(buffer);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("running sketch debug_http.ino");
  delay(3000);

  connectWiFi();
  Serial.println("\nConnected!");
  hsClient.setApiKey(apiKey);
  Serial.println("Hydroserver client is set up");
  Serial.println("posting to hydroserver");
}

void loop() {
  Serial.println("publishing the value");
  double randomValue = random(0, 3000) / 10.0;
  double streamTempRandom = random(10, 20);
  String timestamp = getNextTimestampISO8601();
  temperature.value = randomValue;
  streamTemperature.value = streamTempRandom;
  uint8_t size = sizeof(observations) / sizeof(observations[0]);

  hsClient.publishAll(observations, size, timestamp.c_str());
  Serial.println(timestamp);
  // int status = hsClient.publishObservation(temperature, timestamp.c_str());
  // Serial.println(status);
  // Serial.println(randomValue);
  delay(6000);
}
