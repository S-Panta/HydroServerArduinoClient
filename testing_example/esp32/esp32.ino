#include <Arduino.h>

#define TINY_GSM_MODEM_ESP32
#define XbeeSerial Serial1
#include <TinyGsmClient.h>
#define XBEE_PWR 18


#define TINY_GSM_USE_WIFI true

#include "arduino_secrets.h"

// wifi details
const char* wifiId  = WIFI_SSID;
const char* wifiPwd = WIFI_PASS;

TinyGsm modem(XbeeSerial);
TinyGsmClient client(modem);


const int32_t modemBaud     = 57600;

Observation temperature = {"temperature", "uuidtemperature", "tempsensorid"};


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
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("esp32 test");

  pinMode(XBEE_PWR, OUTPUT);
  digitalWrite(XBEE_PWR, HIGH);

  XbeeSerial.begin(57600); 

  // modem.modemPowerUp();
  delay(3000);               
  Serial.println("powered up module");

  connectWiFi(); 
  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("Arduinopublisher"); 


  if(!mqttClient.connectToBroker()){
    Serial.println("Cannot connect to Broker. Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    // it make no sense to work further when connection to broker is not successful
    while (1);
  };

  Serial.println("connection successful");

  // initialize your sensor
  if (!sht4.begin()) {
        Serial.println("SHT4x sensor not found. Exiting..");
        while (1);
    }
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);     
}


void loop() { 
}


