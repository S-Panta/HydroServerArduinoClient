#define TINY_GSM_MODEM_XBEE
#define XbeeSerial Serial1
#define XBEE_PWR 18
#define TINY_GSM_USE_WIFI true
#include "arduino_secrets.h"

const char *wifiId = WIFI_SSID;
const char *wifiPwd = WIFI_PASS;

#include <HydroServerMQTTClient.h>
#include <TinyGsmClient.h>
TinyGsm modem(XbeeSerial);
TinyGsmClient client(modem);
Observation streamTemperature;

const char *MQTT_BROKER = "raspberrypi1.mypc.usu.edu";
HydroServerMQTTClient mqttClient(client, MQTT_BROKER);

#include "Sodaq_DS3231.h"

void connectWiFi() {
  Serial.println("connecting to wifi");
  Serial.println(wifiId);
  Serial.println(wifiPwd);
  if (!modem.networkConnect(wifiId, wifiPwd)) {
    Serial.println("wifi is not connected");
  }
  Serial.println("Wifi is connected");
  if (!modem.waitForNetwork()) {
    Serial.println("Wifi is not connected");
    delay(10000);
    while (1)
      ;
  }
  Serial.print("Local IP: ");
  Serial.println(modem.localIP());
}

char *getISO8601Time() {

  DateTime now = rtc.now();
  // create a temporary buffer to put the timestamp into
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(),
           now.month(), now.date(), now.hour(), now.minute(), now.second());
  return buf;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("setting up rtc");
  pinMode(XBEE_PWR, OUTPUT);
  digitalWrite(XBEE_PWR, HIGH);
  delay(2000);
  XbeeSerial.begin(9600);
  delay(3000);
  connectWiFi();
  // 132, 163, 97, 1
  Serial.println("Connecting to time server");
  bool connected = timeClient.connect("132.163.97.1", 37);
  if (connected) {
    timeClient.println('!');
    delay(5000);
    if (timeClient.available() >= 4) {
      uint8_t buffer[4]; // Array declaration
      timeClient.readBytes(buffer, 4);
      timeClient.stop();
      uint32_t ntpTime = ((uint32_t)buffer[0] << 24) |
                         ((uint32_t)buffer[1] << 16) |
                         ((uint32_t)buffer[2] << 8) | (uint32_t)buffer[3];
      uint32_t unixTime = ntpTime - 2208988800UL;
      // DS3231 chip give the number of seconds since January 1, 2000
      // Unix Time, which is the number of seconds since 1/1/1970
      // https://www.envirodiy.org/ds3231-real-time-clock-rtc-date-conversion/
      DateTime dt(unixTime - 946684800UL);
      rtc.begin();
      rtc.setDateTime(dt);
      Serial.println("datetime server setting completed");
    }

    timeClient.stop();
    delay(3000);

  } else {
    Serial.println("Client couldn't connect to time server");
  }
  // if (client.connected()) {
  //   Serial.println("Still connected");
  // } else {
  //     Serial.println("Connection closed");
  // }

  streamTemperature.observedProperty = "Temperature";
  streamTemperature.datastreamId = "uuid-temperature";
  streamTemperature.sensorId = "temp-sensor";
  mqttClient.setClientID("mayfly-enlab");
  // make sure to provide sitecode or else mqtt willnot work
  mqttClient.setSiteCode("urwl");
  Serial.println("Connecting to broker");
  if (!mqttClient.connectToBroker()) {
    Serial.println("Cannot connect to Broker. Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    // it make no sense to work further when connection to broker is not
    // successful
    while (1)
      ;
  };
  Serial.println("Connection to Broker Successful");
}

void loop() {
  Serial.println("publishing measurement");
  mqttClient.poll();
  float temperature;
  temperature = random(20, 30);
  streamTemperature.value = temperature;

  // mqttClient.publishObservation(streamTemperature,"2026-06-07T14:30:00Z");
  mqttClient.publishObservation(streamTemperature, getISO8601Time());
  delay(10000);
}
