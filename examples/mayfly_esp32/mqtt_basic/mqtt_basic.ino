#include <Arduino.h>

#define XbeeSerial Serial1
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

// for esp32, modembaud should be 57600
const int32_t modemBaud = 57600;

ExpressifESP32 esp32(XbeeSerial, XBEE_PWR);
// for debugging
// ExpressifESP32 esp32(XbeeSerial, XBEE_PWR,Serial);

// Create an extra reference to the modem by a generic name
ExpressifESP32 &modem = esp32;

HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

Observation temperature;

// --- interval publishing (ModularSensors-style) ---
const uint32_t loggingIntervalMinutes = 1;  // change to 5, 15, etc. as needed
uint32_t lastPublishedEpoch = 0;

void setupDateTimeFromServer(uint32_t unix_time) {
  rtc.begin();
  rtc.setDateTime(unix_time);
}

char *getISO8601Time(DateTime &now) {
  // create a temporary buffer to put the timestamp into
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(),
           now.month(), now.date(), now.hour(), now.minute(), now.second());
  return buf;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);

  XbeeSerial.begin(modemBaud);

  modem.powerUp();
  delay(1000);

  Serial.println("powered up module");
  if (!modem.connectToInternet(wifiId, wifiPwd)) {
    Serial.println("Wifi is not connected");
  }
  Serial.println("Wifi is connected");
  delay(2000);
  uint32_t datetime = modem.getNISTTime();
  setupDateTimeFromServer(datetime);
  Serial.println("datetime setup from server completed");

  // This is important to make sure your mqtt works with esp32
  modem.extraSetupForMQTT();

  Serial.println("Initializing broker connection");
  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("Arduinopublisher");
  mqttClient.setKeepAliveInterval(100);
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
  mqttClient.poll();

  DateTime now = rtc.now();
  uint32_t markedEpochTime = now.getEpoch();

  // fires exactly on the clock boundary (e.g. :00 of every interval-th minute)
  bool onInterval =
      (markedEpochTime != 0) &&
      (markedEpochTime % (loggingIntervalMinutes * 60) == 0);

  // guard so we only publish once per boundary, not for the whole second
  if (onInterval && markedEpochTime != lastPublishedEpoch) {
    lastPublishedEpoch = markedEpochTime;

    float randomTemp = random(20, 25);
    temperature.value = randomTemp;
    mqttClient.publishObservation(temperature, getISO8601Time(now));

    Serial.println("published");
  }
}