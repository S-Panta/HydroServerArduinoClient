#define XbeeSerial Serial1
#define XBEE_PWR 18

#include "arduino_secrets.h"
#include <HydroServerMQTTClient.h>
#include <modems/DigiXbeeS6B.h>

// for internal sensor in mayfly
#include "Adafruit_SHT4x.h"
#include <Wire.h>

// wifi details
const char *wifiId = WIFI_SSID;
const char *wifiPwd = WIFI_PASS;

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
DigiXbeeS6B digixbee(XbeeSerial,XBEE_PWR);
DigiXbeeS6B &modem = digixbee;

HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

const char *datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";
Observation streamTemperature;

// initialize sensor
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 10000;

// for realtime clock
#include <Sodaq_DS3231.h>

char *getISO8601Time() {
  DateTime now = rtc.now();
  // create a temporary buffer to put the timestamp into
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(),
           now.month(), now.date(), now.hour(), now.minute(), now.second());
  return buf;
}

void setupDateTimeFromServer(uint32_t unix_time) {
  rtc.begin();
  rtc.setDateTime(unix_time);
}

void setup() {

  Serial.begin(115200);
  delay(2000);

  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);

  XbeeSerial.begin(9600);
  delay(3000);

  modem.powerUp();
  delay(2000);


  if (!modem.connectToInternet("wifiId", wifiPwd)) {
    Serial.println("Wifi is not connected");
  }

  // if (!modem.connectToInternet(wifiId, wifiPwd)) {
  //   Serial.println("Wifi is not connected");
  // }

  Serial.println("Wifi is connected");
}

void loop() {
}
