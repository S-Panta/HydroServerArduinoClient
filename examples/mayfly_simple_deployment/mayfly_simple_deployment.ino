#define XbeeSerial Serial1
#define XbeePower 18
#define TINY_GSM_MODEM_XBEE

#include <HydroServerMQTTClient.h>
#include <TinyGsmClient.h>
#define TINY_GSM_USE_WIFI true

// for internal sensor in mayfly
#include "Adafruit_SHT4x.h"
#include <Wire.h>

// for debug
// #define TINY_GSM_DEBUG Serial
// for mayfly, it is recommended to use hostname
// if ip is used, the ip should be updated in DL using xctu
// const char* MQTT_BROKER   = "54.36.178.49"; 
// const char* MQTT_BROKER   = "broker.hivemq.com"; 
const char *MQTT_BROKER = "raspberrypi1.mypc.usu.edu";

// uncomment this to see debug from TinyGSM client
// #include <StreamDebugger.h>
// StreamDebugger debugger(XbeeSerial, Serial);
// TinyGsm        modem(debugger);

// initialize client
TinyGsm modem(XbeeSerial);
TinyGsmClient client(modem);
HydroServerMQTTClient mqttClient(client, MQTT_BROKER);

const char *datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";
Observation streamTemperature;

// Observation streamTemperature = { "Temperature", "uuid-temperature"};

// initialize sensor
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// for realtime clock
#include <Sodaq_DS3231.h>

void connectToWifi() {
  // using xbee, the wifi should be preconfigured with xctu.
  Serial.println("connecting to wifi");
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
}

String getISO8601Timestamp() {
  DateTime now = rtc.now();
  char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(),
           now.month(), now.date(), now.hour(), now.minute(), now.second());
  return String(buf);
}

void setup() {

  Serial.begin(115200);
  delay(2000);
  pinMode(XbeePower, OUTPUT);
  digitalWrite(XbeePower, HIGH);
  delay(2000);

  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);

  XbeeSerial.begin(9600);
  delay(3000);

  connectToWifi();
  streamTemperature.observedProperty = "Temperature";
  streamTemperature.datastreamId = "uuid-temperature";

  mqttClient.setKeepAliveInterval(150000UL);
  mqttClient.setClientID("mayfly-enlab");
  mqttClient.setSiteCode("urwl");
  if (!mqttClient.connectToBroker()) {
    Serial.println("Cannot connect to Broker. Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    // it make no sense to work further when connection to broker is not
    // successful
    while (1)
      ;
  };

  Serial.println("Connection to Broker Successful");

  if (!sht4.begin()) {
    Serial.println("SHT4x sensor not found");
    while (1)
      ;
  }
  Serial.println("Found SHT4x sensor");
  // three precision is available

  rtc.begin();
  rtc.setDateTime(DateTime(2026, 7, 7, 14, 30, 0, 2));
}

void loop() {
  mqttClient.poll();
  sensors_event_t humidity, temp;

  mqttClient.poll();
  sht4.getEvent(&humidity, &temp);
  String currentTimeStamp = getISO8601Timestamp();
  streamTemperature.value = temp.temperature;
  mqttClient.publishObservation(streamTemperature, currentTimeStamp.c_str());
  Serial.println(mqttClient.getConnectionError());
  delay(5000);
}
