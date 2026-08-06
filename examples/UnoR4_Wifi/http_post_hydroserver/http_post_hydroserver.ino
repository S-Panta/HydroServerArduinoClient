// for arduino uno
#include "arduino_secrets.h"
#include <HydroServerHTTPClient.h>
#include <WiFiS3.h>

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

// // for playground
const char *apiKey = PLAYGROUND_API_KEY;
const char *datastreamId = "019f246b-c5b9-7b45-aac6-261adc526b55";
const char *serverAddress = "playground.hydroserver.org";
const int serverPort = 443;
WiFiClient wifiClient;
WiFiSSLClient sslClient;
HydroServerHTTPClient hsClient(sslClient, serverAddress, serverPort);

// for local
// const char *serverAddress = "144.39.67.171";
// const int serverPort = 80;
// const char *apiKey = LOCAL_API_KEY;
// const char *datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";
// WiFiClient wifiClient;
// HydroServerHTTPClient hsClient(wifiClient, serverAddress, serverPort);

const long interval = 30000;
unsigned long previousMillis = 0;

Observation temperature = {"Temperature", datastreamId};
const char *streamTempDataStreamId = "019fa988-631d-7dc1-886f-b80431339ff7";
Observation streamTemperature = {"StreamTemperature", streamTempDataStreamId};

Observation *observations[] = {&temperature, &streamTemperature};

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

// Must run after Wi-Fi is connected and before any publish
void syncRTCFromNTP() {
  timeClient.update();
  unsigned long unixTime = timeClient.getEpochTime();
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);
  Serial.print("RTC set to: ");
  Serial.println(timeClient.getFormattedTime());
}

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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);
  delay(3000);

  connectWiFi();

  // setting time from server
  timeClient.update();

  RTC.begin();
  syncRTCFromNTP();
  Serial.println("ntp time from server setup completed");

  hsClient.setApiKey(apiKey);
  Serial.println("Hydroserver client is set up");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    Serial.println("publishing the value");
    previousMillis = currentMillis;
    double randomValue = random(0, 3000) / 10.0;
    double streamTempRandom = random(10, 20);
    temperature.value = randomValue;
    streamTemperature.value = streamTempRandom;

    uint8_t size = sizeof(observations) / sizeof(observations[0]);

    // Serial.println(hsClient.publishAll(observations,size,getISO8601Time()));
    Serial.println(hsClient.publishObservation(temperature, getISO8601Time()));

    Serial.println(hsClient.getStatusCode());
    Serial.println(hsClient.getResponseBody());
  };
}
