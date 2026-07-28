#define TINY_GSM_MODEM_ESP32
#define XbeeSerial Serial1
#define XBEE_PWR 18
#define TINY_GSM_USE_WIFI true
#include "arduino_secrets.h"

#include <HydroServerMQTTClient.h>
#include <StreamDebugger.h>
#include <TinyGsmClient.h>
// StreamDebugger debugger(Serial1, Serial);
// TinyGsm        modem(debugger);
TinyGsm modem(XbeeSerial);
TinyGsmClient client(modem);
Observation streamTemperature;

const char *MQTT_BROKER = "raspberrypi1.mypc.usu.edu";
HydroServerMQTTClient mqttClient(client, MQTT_BROKER);

#include "Sodaq_DS3231.h"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

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
  Serial.println("connecting to wifi");
  if (!modem.networkConnect(ssid, password)) {
    Serial.println("wifi is not connected");
    while (1)
      ;
  }

  Serial.println("Wifi is connected");

  if (modem.waitForNetwork()) {
    Serial.println("Network connected");
  }

  Serial.print("Local IP: ");
  Serial.println(modem.localIP());
  delay(3000);
}

void getNetworkTime() {
  int ntp_year, ntp_month, ntp_day;
  int ntp_hour, ntp_min, ntp_sec;
  float ntp_timezone;

  if (modem.getNetworkTime(&ntp_year, &ntp_month, &ntp_day, &ntp_hour, &ntp_min,
                           &ntp_sec, &ntp_timezone)) {

    Serial.print("Date: ");
    Serial.print(ntp_year);
    Serial.print("-");
    Serial.print(ntp_month);
    Serial.print("-");
    Serial.println(ntp_day);

    Serial.print("Time: ");
    Serial.print(ntp_hour);
    Serial.print(":");
    Serial.print(ntp_min);
    Serial.print(":");
    Serial.println(ntp_sec);

    Serial.print("Timezone: ");
    Serial.println(ntp_timezone);
    rtc.begin();
    // Set RTC
    DateTime dt(ntp_year, ntp_month, ntp_day, ntp_hour, ntp_min, ntp_sec, 0);
    rtc.setDateTime(dt);
    Serial.println("datetime updated");

  } else {
    Serial.println("Failed to get network time");
  }
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
  pinMode(XBEE_PWR, OUTPUT);
  digitalWrite(XBEE_PWR, HIGH);
  delay(2000);
  XbeeSerial.begin(57600);
  modem.init();
  delay(3000);
  connectWiFi();
  Serial.println("powering the modem");

  Serial.println("setting up rtc");
  modem.NTPServerSync("pool.ntp.org");

  modem.waitForTimeSync();
  getNetworkTime();
  // This is important to make sure your mqtt works with esp32
  sendATCommand("AT+CIPRECVMODE=1");

  streamTemperature.observedProperty = "Temperature";
  streamTemperature.datastreamId = "uuid-temperature";
  streamTemperature.sensorId = "temp-sensor";
  mqttClient.setClientID("mayfly-enlab-test");
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