#define TINY_GSM_MODEM_XBEE
#define TINY_GSM_RX_BUFFER 256
#define XbeeSerial Serial1
#define XBEE_PWR 18

#include <TinyGsmClient.h>
#include <HydroServerMQTTClient.h>
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#include <Wire.h> 
#include "Adafruit_SHT4x.h"


Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// wifi details
const char* ssid = "USU-guest";
const char* password = 0;

// use broker host or ip
// for mayfly, it is recommended to use hostname
// if ip is used, the ip should be updated in DL using xctu
const char* MQTT_BROKER = "raspberrypi1.mypc.usu.edu";
const int   MQTT_PORT     = 1883;

// initialize the client
TinyGsm modem(XbeeSerial);
TinyGsmClient client(modem);
HydroServerMQTTClient mqttClient(client,MQTT_BROKER,MQTT_PORT);  


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


void onMqttMessage(int messageSize) {
    String topic   = mqttClient.messageTopic(); 
    String payload = mqttClient.readMessage(); 

    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.println(payload);
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Mayfly sketch for publishing Measured Temperature");

  pinMode(XBEE_PWR, OUTPUT);
  digitalWrite(XBEE_PWR, HIGH);
  delay(2000);
  XbeeSerial.begin(9600);
  delay(3000);

  connectWiFi();

  Serial.println("connecting to broker");
  // The callback in this will tell the device to do what message is received from subscribed topic
  mqttClient.onMessage(onMqttMessage);

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
  // mqttClient.subscribe("rainfall");
}

float preparePayload(){
  sensors_event_t humidity,temperature;
  // new fresh data for temperature and humidity
  sht4.getEvent(&humidity,&temperature);
  return temperature.temperature; 
}

void loop() {
  // poll is necessary so as to fire the callback
  // mqttClient.poll();
  Serial.println(preparePayload());
  mqttClient.publishData("uwrl/mayfly/water/temperature", String(preparePayload()).c_str());
  // // publish every 5 seconds
  delay(5000);  
}