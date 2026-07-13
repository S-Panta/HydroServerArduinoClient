// for arduino uno
#include "arduino_secrets.h"
#include <HydroServerHTTPClient.h>
#include <WiFiS3.h>

// for mayfly
// #define TINY_GSM_MODEM_XBEE
// #define TINY_GSM_RX_BUFFER 256
// #define XBEE_PWR 18
// #define XbeeSerial Serial1
// #include <TinyGsmClient.h>
// #include <ArduinoHttpClient.h>

// #define TINY_GSM_USE_WIFI true
// #include "arduino_secrets.h"
// #include <ArduinoJson.h>

const char *ssid = "USU-guest";
const char *password = 0;

unsigned long lastPostTime = 0;
const unsigned long postInterval = 20000;
int currentHour = 3;

// const char* serverAddress = "d1yuif7k84op9r.cloudfront.net";
const char *serverAddress = "playground.hydroserver.org";
// // const char* serverAddress = "129.123.0.1";
// // const char* serverAddress = "localhost";
const int serverPort = 443;
// const char* apiPath       = "/api/sensorthings/v1.1/Observations";
// // for playground
const char *apiKey = "MVZDruGZXYcp44Pgcb9nPetpIUl84mqlcQzvfJwUj8hfIz1qpElvNXQ";

const char *datastreamId = "019f246b-c5b9-7b45-aac6-261adc526b55";

// for arduino uno
WiFiClient wifiClient;
WiFiSSLClient sslClient;
HydroServerHTTPClient hsClient(sslClient, serverAddress, serverPort);
Observation temperature = {"Temperature", datastreamId};

// HttpClient httpClient(sslClient,serverAddress,serverPort);

// for mayfly

// #include <StreamDebugger.h>
// StreamDebugger debugger(XbeeSerial, Serial);
// TinyGsm        modem(debugger);

// TinyGsm modem(XbeeSerial);
// TinyGsmClientSecure client(modem);
// HttpClient    httpClient(client, serverAddress, serverPort);

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
  snprintf(buffer, sizeof(buffer), "2026-07-08T%02d:59:43Z", currentHour);
  currentHour = (currentHour + 1) % 24;
  return String(buffer);
}

// for mayfly
// void connectWiFi() {
//   if (!modem.waitForNetwork()) {
//     Serial.println("Wifi is not connected");
//     delay(10000);
//     return;
//   }

//   Serial.println("Wifi is connected");

//   if (modem.isNetworkConnected()) { Serial.println("Network connected"); }

//   Serial.print("Local IP: ");
//   Serial.println(modem.localIP());
// }

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("running sketch debug_http.ino");
  // pinMode(XBEE_PWR, OUTPUT);
  // digitalWrite(XBEE_PWR, HIGH);
  // delay(2000);
  // XbeeSerial.begin(9600);
  delay(3000);

  connectWiFi();
  Serial.println("\nConnected!");
  hsClient.setApiKey(apiKey);
  Serial.println("Hydroserver client is set up");
}

void loop() {
  if (millis() - lastPostTime >= postInterval) {
    lastPostTime = millis();

    double randomValue = random(0, 3000) / 10.0;
    String timestamp = getNextTimestampISO8601();
    temperature.value = randomValue;

    int status = hsClient.publishObservation(temperature, timestamp.c_str());
    Serial.println(randomValue);

    Serial.print(hsClient.getResponseBody());
  }
}
