#include <Arduino.h>

// for esp32
#define TINY_GSM_MODEM_ESP32

// for xbee
// #define TINY_GSM_MODEM_XBEE
#define XbeeSerial Serial1
// for debugging
// #define TINY_GSM_DEBUG Serial
#include <TinyGsmClient.h>
#define XBEE_PWR 18

#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true

#include "arduino_secrets.h"
#include <HydroServerHTTPClient.h>

// wifi details
const char *wifiId = "USU-guest";
const char *wifiPwd = 0;

// for playground
const char *apiKey = PLAYGROUND_API_KEY;
const char *datastreamId = "019f246b-c5b9-7b45-aac6-261adc526b55";
const char *serverAddress = "playground.hydroserver.org";
const int serverPort = 443;

const int mux = 0;
// for debugging
#include <StreamDebugger.h>
// StreamDebugger debugger(Serial1, Serial);
// TinyGsm        modem(debugger);

// for normal mode
TinyGsm modem(XbeeSerial);
// TinyGsmClient client(modem);

TinyGsmClientSecure sslClient(modem, mux);

HydroServerHTTPClient hsClient(sslClient, serverAddress, serverPort);
Observation temperature;
int currentHour = 3;

String sendATCommand(String cmd, uint32_t timeout_ms = 2000) {
  while (XbeeSerial.available()) {
    XbeeSerial.read();
  }

  Serial.print(">> ");
  Serial.println(cmd);

  XbeeSerial.print(cmd);
  XbeeSerial.print("\r\n");

  String response = "";
  uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    while (XbeeSerial.available()) {
      char c = XbeeSerial.read();
      response += c;
      start = millis();
    }
    if (response.endsWith("OK\r\n") || response.endsWith("ERROR\r\n")) {
      break;
    }
  }

  Serial.print("<< ");
  Serial.println(response);
  Serial.println("....................................");

  return response;
}

void connectWiFi() {

  // the wifi setup is needed only for instanting modem for first time
  // Serial.print(F("Setting SSID/password..."));
  // if (!modem.networkConnect(wifiId, 0)) {
  //   Serial.println(" fail");
  //   delay(10000);
  //   while(1);
  // }

  if (!modem.waitForNetwork()) {
    Serial.println("Wifi is not connected");
    delay(10000);
    while (1)
      ;
  }

  Serial.println("Wifi is connected");

  if (modem.isNetworkConnected()) {
    Serial.println("Network connected");
  }

  Serial.print("Local IP: ");
  Serial.println(modem.localIP());

  // sendATCommand("AT+CIPRECVMODE?");
  // This is important to make sure your mqtt works with esp32
  sendATCommand("AT+CIPRECVMODE=1");

  Serial.println("the firmware set to passive");
  delay(3000);

  // Some HTTPS servers host multiple domains on the same IP address.
  // During the TLS handshake, the modem must send the hostname using
  // Server Name Indication so the server can present the correct
  // SSL/TLS certificate. Without SNI, the connection may fail even
  // though DNS resolution and TCP connectivity succeed.
  // I don't know how this works but this need to be done twice
  // 0 and 1 connection ID needs SNI
  sendATCommand("AT+CIPSSLCSNI=0,\"playground.hydroserver.org\"");
  sendATCommand("AT+CIPSSLCSNI=1,\"playground.hydroserver.org\"");
  4 delay(3000);
  // same as above. This also need to be done twice
  sendATCommand("AT+CIPSTART=0,\"SSL\",\"playground.hydroserver.org\",443");
  sendATCommand("AT+CIPSTART=1,\"SSL\",\"playground.hydroserver.org\",443");
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
  Serial.println("esp32 test");

  pinMode(XBEE_PWR, OUTPUT);
  digitalWrite(XBEE_PWR, HIGH);
  // For xbee, comment this line out, xbee runs on 9600 baud rate
  XbeeSerial.begin(57600);
  // XbeeSerial.begin(9600);

  delay(3000);
  Serial.println("powered up module");

  // For xbee, comment this line out
  modem.init();

  connectWiFi();
  hsClient.setApiKey(apiKey);

  temperature.datastreamId = datastreamId;
  temperature.sensorId = "sensorid";
  temperature.observedProperty = "Temperature";

  Serial.println("posting to hydroserver");
  double randomValue = random(0, 3000) / 10.0;
  String timestamp = getNextTimestampISO8601();
  temperature.value = randomValue;

  int status = hsClient.publishObservation(temperature, timestamp.c_str());
  Serial.println(status);
  Serial.print(hsClient.getResponseBody());
  Serial.println(randomValue);
}

void loop() {}
