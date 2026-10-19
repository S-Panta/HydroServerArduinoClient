// ============================================================
// http_post_hydroserver.ino
//
// EnviroDIY Mayfly + ESP32 Wi-Fi Bee: post a simulated temperature
// observation to a HydroServer instance over plain HTTP (port 80) every
// 30 seconds. Use this for a local/self-hosted HydroServer without TLS;
// for the hosted playground use https_post_hydroserver instead.
//
// Hardware:  EnviroDIY Mayfly v1.x, ESP32 Bee (Espressif AT firmware)
// Libraries: HydroServerArduinoClient, ArduinoHttpClient, ArduinoJson,
//            EnviroDIY TinyGSM fork, StreamDebugger, Sodaq_DS3231
//
// Before uploading:
//   1. Create arduino_secrets.h next to this sketch with
//        #define WIFI_SSID "your-network"
//        #define WIFI_PASS "your-password"   // use 0 for an open network
//        #define LOCAL_API_KEY "your-hydroserver-api-key"
//   2. Set SERVER_HOST and DATASTREAM_ID below.
// ============================================================

#include "arduino_secrets.h"
#include <HydroServerHTTPClient.h>
#include <Sodaq_DS3231.h>
#include <modems/ExpressifESP32.h>

// ---- Configuration -------------------------------------------------------
// Hostname or IP address of the machine running HydroServer, as seen from
// the logger's network. "localhost" would point the logger at itself.
const char *SERVER_HOST = "192.168.1.100";
const uint16_t SERVER_PORT = 80;
const char *DATASTREAM_ID = "YOUR-TEMPERATURE-DATASTREAM-UUID";

const unsigned long PUBLISH_INTERVAL_MS = 30000;

// ---- Hardware ------------------------------------------------------------
HardwareSerial &beeSerial = Serial1; // Bee socket serial port
const int8_t BEE_POWER_PIN = 18;     // Bee socket power pin on Mayfly v1.x
const uint32_t BEE_BAUD = 57600;     // ESP32 AT firmware baud rate

ExpressifESP32 modem(beeSerial, BEE_POWER_PIN);
// ExpressifESP32 modem(beeSerial, BEE_POWER_PIN, Serial); // echo AT traffic

// Plain HTTP uses the normal TCP client. createClient() allocates, so call
// it once.
HydroServerHTTPClient hsClient(*modem.createClient(), SERVER_HOST, SERVER_PORT,
                               LOCAL_API_KEY);

Observation temperature = {"temperature", DATASTREAM_ID, "temp-sensor-1"};

unsigned long lastPublishMs = 0;

// ---- Helpers -------------------------------------------------------------
const char *getISO8601Time() {
  DateTime now = rtc.now();
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(),
           now.month(), now.date(), now.hour(), now.minute(), now.second());
  return buf;
}

// ---- Sketch --------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  beeSerial.begin(BEE_BAUD);

  modem.powerUp();
  delay(1000); // give the ESP32 time to boot

  Serial.println("Connecting to Wi-Fi");
  if (!modem.connectToInternet(WIFI_SSID, WIFI_PASS)) {
    Serial.println("Wi-Fi connection failed");
    while (true) {
    }
  }
  Serial.println("Wi-Fi connected");

  rtc.begin();
  rtc.setDateTime(modem.getNISTTime());
  Serial.print("Clock set to ");
  Serial.println(getISO8601Time());

  // Puts the ESP32 in passive receive mode (AT+CIPRECVMODE=1). Despite the
  // name, plain TCP connections such as HTTP need this too.
  modem.extraSetupForMQTT();

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
    temperature.value = random(200, 251) / 10.0;

    if (hsClient.publishObservation(temperature, getISO8601Time())) {
      Serial.println("Observation posted");
    } else {
      Serial.print("Post failed, server replied: ");
      Serial.println(hsClient.getLastResponseBody());
    }
  }
}
