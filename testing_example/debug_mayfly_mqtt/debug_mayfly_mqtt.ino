#define XbeeSerial Serial1
#define XbeePower 18
#include "arduino_secrets.h"
#include "Sodaq_DS3231.h"

// #include <HydroServerMQTTClient.h>
const char *MQTT_BROKER = "raspberrypi1.mypc.usu.edu";
// #define TINY_GSM_MODEM_XBEE

// wifi details
const char *wifiId = WIFI_SSID;
const char *wifiPwd = WIFI_PASS;

#include <HydroServerMQTTClient.h>
// #include <TinyGsmClient.h>
// #define TINY_GSM_USE_WIFI true

// #include <modems/DigiXbeeS6B.h>

#include <modems/ExpressifESP32.h>


ExpressifESP32 xbee(XbeeSerial,XbeePower);
HydroServerMQTTClient mqttClient(
    *xbee.createClient(),
    MQTT_BROKER
);

const char *datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";
Observation streamTemperature;


// void connectToWifi() {
//   // using xbee, the wifi should be preconfigured with xctu.
//   Serial.println("connecting to wifi");
//   if (!modem.waitForNetwork()) {
//     Serial.println("Wifi is not connected");
//     delay(10000);
//     while (1)
//       ;
//   }

//   Serial.println("Wifi is connected");

//   if (modem.isNetworkConnected()) {
//     Serial.println("Network connected");
//   }

//   Serial.print("Local IP: ");
//   Serial.println(modem.localIP());
// }

// String getISO8601Timestamp() {
//   DateTime now = rtc.now();
//   char buf[25];
//   snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(),
//            now.month(), now.date(), now.hour(), now.minute(), now.second());
//   return String(buf);
// }

void setupDateTimeFromServer(uint32_t unix_time){
  rtc.begin();
  rtc.setDateTime(unix_time);
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
  delay(2000);
  // XbeeSerial.begin(9600);
  XbeeSerial.begin(57600);
  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);
  // xbee.powerUp();
  if (!xbee.connectToInternet(wifiId, wifiPwd)) {
    Serial.println("Connection failed");
  } else {
    Serial.println("Wifi is connected");
  }

  delay(2000);
  uint32_t datetime = xbee.getNISTTime();
  Serial.println(datetime);
  setupDateTimeFromServer(datetime);
  

  // connectToWifi();
  // streamTemperature.observedProperty = "Temperature";
  // streamTemperature.datastreamId = "uuid-temperature";

  // mqttClient.setKeepAliveInterval(150000UL);
  // mqttClient.setClientID("mayfly-enlab");
  // mqttClient.setSiteCode("urwl");
  // if (!mqttClient.connectToBroker()) {
  //   Serial.println("Cannot connect to Broker. Connection Error is ");
  //   Serial.println(mqttClient.getConnectionError());
  //   // it make no sense to work further when connection to broker is not
  //   // successful
  //   while (1)
  //     ;
  // };

  // Serial.println("Connection to Broker Successful");

  // if (!sht4.begin()) {
  //   Serial.println("SHT4x sensor not found");
  //   while (1)
  //     ;
  // }
  // Serial.println("Found SHT4x sensor");
  // // three precision is available

  // rtc.begin();
  // rtc.setDateTime(DateTime(2026, 7, 7, 14, 30, 0, 2));
}

void loop() {
  Serial.println("printing");
  

  Serial.println(getISO8601Time());
  delay(10000);
  // mqttClient.poll();
  // sensors_event_t humidity, temp;

  // mqttClient.poll();
  // sht4.getEvent(&humidity, &temp);
  // String currentTimeStamp = getISO8601Timestamp();
  // streamTemperature.value = temp.temperature;
  // mqttClient.publishObservation(streamTemperature, currentTimeStamp.c_str());
  // Serial.println(mqttClient.getConnectionError());
  // delay(5000);
}
