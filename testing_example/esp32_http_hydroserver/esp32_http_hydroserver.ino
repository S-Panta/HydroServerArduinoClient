#include <Arduino.h>

// #define TINY_GSM_MODEM_ESP32
#define TINY_GSM_MODEM_XBEE
#define XbeeSerial Serial1
#define TINY_GSM_DEBUG Serial
#include <TinyGsmClient.h>
#define XBEE_PWR 18




#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true

#include "arduino_secrets.h"
#include <HydroServerHTTPClient.h>


// wifi details
const char* wifiId  = "USU-guest";
const char* wifiPwd = 0;


#include <StreamDebugger.h>
StreamDebugger debugger(Serial1, Serial);
TinyGsm        modem(debugger);

// const char *serverAddress = "playground.hydroserver.org";
const char* serverAddress = "129.123.0.1";
const int serverPort = 80;


// const char *apiKey = "MVZDruGZXYcp44Pgcb9nPetpIUl84mqlcQzvfJwUj8hfIz1qpElvNXQ";

// local instance key
const char *apiKey = "vSvFWf0NeWY1GgNYT0ovGONsmKLEHaTZUTFHTO3_l_PJlv2-TO9R7-0";
const char *datastreamId = "019f246b-c5b9-7b45-aac6-261adc526b55";



// TinyGsm modem(XbeeSerial);
TinyGsmClient client(modem);
// TinyGsmClientSecure sslClient(modem);

HydroServerHTTPClient hsClient(client, serverAddress, serverPort);
Observation temperature;

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

  // Serial.print(F("Setting SSID/password..."));
  // if (!modem.networkConnect(wifiId, 0)) {
  //   Serial.println(" fail");
  //   delay(10000);
  //   while(1);
  // }

  if (!modem.waitForNetwork()) {
    Serial.println("Wifi is not connected");
    delay(10000);
    while(1);
  }

  Serial.println("Wifi is connected");

  if (modem.isNetworkConnected()) { Serial.println("Network connected"); }

  Serial.print("Local IP: ");
  Serial.println(modem.localIP());
  // sendATCommand("AT+CIPMUX=1");
  // Serial.println("CIPMUX is set to 1 in this step");
  // This is done by modem.init()
  // sendATCommand("AT+CIPMUX=1");
  // delay(2000);
  // sendATCommand("AT+CIPRECVMODE?");
  // This is important to make sure your mqtt works with esp32
  // sendATCommand("AT+CIPRECVMODE=1");
  // Serial.println("the firmware set to passive");
  // delay(3000);
  // sendATCommand("AT+CIPRECVMODE?");
  // Serial.println("checking again");
  // sendATCommand("AT+CIPSTART=0,\"TCP\",\"test.mosquitto.org\",1883");
  // Serial.println("tcp connection is openeed in this step");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("esp32 test");

  pinMode(XBEE_PWR, OUTPUT);
  digitalWrite(XBEE_PWR, HIGH);
  // For xbee, comment this line out, xbee runs on 9600 baud rate
  // XbeeSerial.begin(57600); 
  XbeeSerial.begin(9600);

  delay(3000);               
  Serial.println("powered up module");
  // For xbee, comment this line out 
  // modem.init();

  connectWiFi();
  hsClient.setApiKey(apiKey);
  

  Serial.println("connection successful");
  temperature.datastreamId = datastreamId;
  temperature.sensorId = "sensorid";
  temperature.observedProperty = "Temperature";
}


void loop() {
  Serial.println("Injecting data to hydroserver");
  float randomTemp;
  randomTemp = random(20,25);
  temperature.value = randomTemp;
  int status = hsClient.publishObservation(temperature, "2026-06-19T00:00:00Z");
  Serial.println(status);
  Serial.println(randomTemp);

    Serial.print(hsClient.getResponseBody());  
    delay(30000);
}


