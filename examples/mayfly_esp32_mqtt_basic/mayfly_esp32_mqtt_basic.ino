#include <Arduino.h>

#define TINY_GSM_MODEM_ESP32
#define XbeeSerial Serial1
// #define TINY_GSM_DEBUG Serial
#define XBEE_PWR 18

#include "Sodaq_DS3231.h"
#include "arduino_secrets.h"
#include <HydroServerMQTTClient.h>
#include <modems/ExpressifESP32.h>

// wifi details
const char *wifiId = WIFI_SSID;
const char *wifiPwd = WIFI_PASS;

const char *MQTT_BROKER = "raspberrypi1.mypc.usu.edu";
// const char *MQTT_BROKER = "test.mosquitto.org";

// const char *MQTT_BROKER = "192.168.0.101";
const int32_t modemBaud = 57600;

// #include <StreamDebugger.h>
// StreamDebugger debugger(Serial1, Serial);
// TinyGsm modem(debugger);

// TinyGsm modem(XbeeSerial);
// TinyGsmClient client(modem);
ExpressifESP32 esp32(XbeeSerial, XBEE_PWR);
// Create an extra reference to the modem by a generic name
ExpressifESP32 modem = esp32;

HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

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

// void connectWiFi() {
//   //  the wifi setup is needed only for instanting modem for first time
//   Serial.print(F("Setting SSID/password..."));
//   if (!modem.networkConnect(wifiId, wifiPwd)) {
//     Serial.println(" fail");
//     delay(10000);
//     while (1)
//       ;
//   }
//   if (!modem.waitForNetwork()) {
//     Serial.println("Wifi is not connected");
//     delay(10000);
//     return;
//   }

//   Serial.println("Wifi is connected");

//   if (modem.isNetworkConnected()) {
//     Serial.println("Network connected");
//   }

//   Serial.print("Local IP: ");
//   Serial.println(modem.localIP());
//   // sendATCommand("AT+CIPMUX=1");
//   // Serial.println("CIPMUX is set to 1 in this step");

//   // sendATCommand("AT+CIPMUX=1");
//   // delay(2000);
//   sendATCommand("AT+CIPRECVMODE?");
//   // This is important to make sure your mqtt works with esp32
//   // sendATCommand("AT+CIPRECVMODE=1");
//   // Serial.println("the firmware set to passive");
//   // delay(3000);
//   // sendATCommand("AT+CIPRECVMODE?");
//   // Serial.println("checking again");
//   //
//   sendATCommand("AT+CIPSTART=0,\"TCP\",\"raspberrypi1.mypc.usu.edu\",1883");
//   // Serial.println("tcp connection is openeed in this step");
// }

void setupDateTimeFromServer(uint32_t unix_time) {
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
  delay(1000);
  Serial.println("esp32 test");

  XbeeSerial.begin(57600);

  modem.powerUp();
  delay(1000);

  delay(3000);
  Serial.println("powered up module");
  if (!modem.connectToInternet(wifiId, wifiPwd)) {
    Serial.println("Wifi is not connected");
  }
  Serial.println("Wifi is connected");
  delay(2000);
  uint32_t datetime = modem.getNISTTime();
  Serial.println(datetime);
  setupDateTimeFromServer(datetime);

  // This is important to make sure your mqtt works with esp32
  sendATCommand("AT+CIPRECVMODE=1");

  Serial.println("Initializing broker connection");
  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("Arduinopublisher");
  if (!mqttClient.connectToBroker()) {
    Serial.println("Cannot connect to Broker. Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    // it make no sense to work further when connection to broker is not
    // successful
    while (1)
      ;
  };

  Serial.println("connection successful");
  temperature.datastreamId = "data-streamid";
  temperature.sensorId = "sensorid";
  temperature.observedProperty = "temperature_celsius";
}

void loop() {
  Serial.println("publishing information");
  mqttClient.poll();
  float randomTemp;
  randomTemp = random(20, 25);
  temperature.value = randomTemp;
  mqttClient.publishObservation(temperature, getISO8601Time());
  delay(10000);
}
