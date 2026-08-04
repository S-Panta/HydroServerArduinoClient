#include <Arduino.h>

// for esp32
#define TINY_GSM_MODEM_ESP32

// for xbee,delete esp32 and uncomment the line below this
// #define TINY_GSM_MODEM_XBEE
#define XbeeSerial Serial1

// for debugging output of tinygsm
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

// for local instance
/*
  The server address is the url of the host machine where hydroserver is
  deployed. URL of localhost would point the machine to itself (for example
  arduino will point at itself)
*/
const char *serverAddress = "144.39.67.171";
// http port
const int serverPort = 80;
// the api key from hydroserver local instance
// this is stored in arduino_secrets file
const char *apiKey = LOCAL_API_KEY;

// the datastream id from hydroserver
const char *datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";

// for debugging of AT commands
#include <StreamDebugger.h>
// StreamDebugger debugger(Serial1, Serial);
// TinyGsm        modem(debugger);

// for normal mode
TinyGsm modem(XbeeSerial);
TinyGsmClient client(modem);

HydroServerHTTPClient hsClient(client, serverAddress, serverPort);
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
  // this line is not needed for xbee s6b wifi module as wifi setup is already
  // done in the modem Serial.print(F("Setting SSID/password...")); if
  // (!modem.networkConnect(wifiId, 0)) {
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

  // do this only when the debugger is on
  // String cmd = String("AT+CIPSTART=0,\"TCP\",\"") + serverAddress + "\"," +
  // String(serverPort); sendATCommand(cmd);
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
  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);

  pinMode(XBEE_PWR, OUTPUT);
  digitalWrite(XBEE_PWR, HIGH);

  // For xbee, comment this line out, xbee runs on 9600 baud rate
  XbeeSerial.begin(57600);
  // XbeeSerial.begin(9600);

  delay(3000);

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

void loop() {
  // Serial.println("Injecting data to hydroserver");
  // float randomTemp;
  // randomTemp = random(20,25);
  // temperature.value = randomTemp;
  // int status = hsClient.publishObservation(temperature,
  // "2026-06-19T00:00:00Z"); Serial.println(status);
  // Serial.println(randomTemp);

  //   Serial.print(hsClient.getResponseBody());
  //   delay(30000);
}
