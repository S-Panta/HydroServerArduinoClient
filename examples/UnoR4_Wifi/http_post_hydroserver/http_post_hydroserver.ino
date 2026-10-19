// ============================================================
// http_post_hydroserver.ino
//
// Arduino Uno R4 WiFi: post a simulated temperature observation straight
// to HydroServer's SensorThings API over HTTPS every 30 seconds.
//
// Hardware:  Arduino Uno R4 WiFi
// Libraries: HydroServerArduinoClient, ArduinoHttpClient, ArduinoJson,
//            NTPClient
//
// Before uploading:
//   1. Create arduino_secrets.h next to this sketch with
//        #define WIFI_SSID "your-network"
//        #define WIFI_PASS "your-password"
//        #define PLAYGROUND_API_KEY "your-hydroserver-api-key"
//   2. Set DATASTREAM_ID to a datastream your API key can write to.
//
// For a local HydroServer over plain HTTP, use WiFiClient instead of
// WiFiSSLClient, set SERVER_PORT to 80 and SERVER_HOST to the server's
// hostname or IP address (not "localhost").
// ============================================================

#include "arduino_secrets.h"
#include <HydroServerHTTPClient.h>
#include <NTPClient.h>
#include <RTC.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

// ---- Configuration -------------------------------------------------------
const char *SERVER_HOST = "playground.hydroserver.org"; // no https://
const uint16_t SERVER_PORT = 443;
const char *DATASTREAM_ID = "01a0025e-8eaf-72c9-9ce6-6d23f48fd4b2";

const unsigned long PUBLISH_INTERVAL_MS = 30000;

// ---- Objects -------------------------------------------------------------
// Port 443 needs a TLS client
WiFiSSLClient sslClient;
HydroServerHTTPClient hsClient(sslClient, SERVER_HOST, SERVER_PORT,
                               PLAYGROUND_API_KEY);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

Observation temperature = {"temperature", DATASTREAM_ID, "temp-sensor-1"};

unsigned long lastPublishMs = 0;

// ---- Helpers -------------------------------------------------------------
// Returns the RTC time as an ISO 8601 UTC string, e.g. 2026-09-23T18:30:00Z.
const char *getISO8601Time() {
  RTCTime t;
  RTC.getTime(t);
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t.getYear(),
           Month2int(t.getMonth()), t.getDayOfMonth(), t.getHour(),
           t.getMinutes(), t.getSeconds());
  return buf;
}

void connectWiFi() {
  Serial.print("Connecting to Wi-Fi ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
    delay(500);
  }
  Serial.print("Wi-Fi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void syncRTCFromNTP() {
  timeClient.begin();
  while (!timeClient.update()) {
    delay(500);
  }
  RTC.begin();
  RTCTime now(timeClient.getEpochTime());
  RTC.setTime(now);
  Serial.print("RTC set to ");
  Serial.println(getISO8601Time());
}

// ---- Sketch --------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000) {
  }

  connectWiFi();
  syncRTCFromNTP();

  // Catches a mistyped UUID or an API key without access before posting
  if (!hsClient.isDatastreamAvailable(DATASTREAM_ID)) {
    Serial.println("Datastream not found or not readable with this API key:");
    Serial.println(hsClient.getLastResponseBody());
    while (true) {
    }
  }
  Serial.println("Datastream found, posting observations");
}

void loop() {
  if (millis() - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = millis();

    // Simulated reading: replace with your own sensor code
    temperature.value = random(0, 3000) / 100.0;

    if (hsClient.publishObservation(temperature, getISO8601Time())) {
      Serial.println("Observation posted");
    } else {
      Serial.print("Post failed, server replied: ");
      Serial.println(hsClient.getLastResponseBody());
    }
  }
}
