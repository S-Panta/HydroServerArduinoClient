#include <Arduino.h>

#define TINY_GSM_MODEM_ESP32
#define XbeeSerial Serial1
// #define TINY_GSM_DEBUG Serial
#include <TinyGsmClient.h>
#define XBEE_PWR 18




#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true

#include "arduino_secrets.h"
#include <HydroServerMQTTClient.h>


// wifi details
const char* wifiId  = WIFI_SSID;
const char* wifiPwd = WIFI_PASS;


const char *MQTT_BROKER = "raspberrypi1.mypc.usu.edu";
const int32_t modemBaud     = 57600;

// #include <StreamDebugger.h>
// StreamDebugger debugger(Serial1, Serial);
// TinyGsm        modem(debugger);

TinyGsm modem(XbeeSerial);
TinyGsmClient client(modem);

HydroServerMQTTClient mqttClient(client, MQTT_BROKER);

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
  if (!modem.waitForNetwork()) {
    Serial.println("Wifi is not connected");
    delay(10000);
    return;
  }

  Serial.println("Wifi is connected");

  if (modem.isNetworkConnected()) { Serial.println("Network connected"); }

  Serial.print("Local IP: ");
  Serial.println(modem.localIP());
  // sendATCommand("AT+CIPMUX=1");
  // Serial.println("CIPMUX is set to 1 in this step");

  // sendATCommand("AT+CIPMUX=1");
  // delay(2000);
  // sendATCommand("AT+CIPRECVMODE?");
  // This is important to make sure your mqtt works with esp32
  sendATCommand("AT+CIPRECVMODE=1");
  Serial.println("the firmware set to passive");
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

  XbeeSerial.begin(57600); 

  delay(3000);               
  Serial.println("powered up module");
  modem.init();

  connectWiFi();
  Serial.println("Now broker connection step has started");
  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("Arduinopublisher");
  if(!mqttClient.connectToBroker()){
    Serial.println("Cannot connect to Broker. Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    // it make no sense to work further when connection to broker is not successful
    while (1);
  };

  Serial.println("connection successful");
  temperature.datastreamId = "data-streamid";
  temperature.sensorId = "sensorid";
  temperature.observedProperty = "temperature_celsius";
}


void loop() {
  mqttClient.poll();
  float randomTemp;
  randomTemp = random(20,25);
  temperature.value = randomTemp;
  mqttClient.publishObservation(temperature, "2026-06-15T00:00:00Z");
  delay(30000);
}


