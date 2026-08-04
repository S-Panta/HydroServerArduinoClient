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

// for mayfly, it is recommended to use ip
// const char* MQTT_BROKER   = "54.36.178.49";
// const char* MQTT_BROKER   = "broker.hivemq.com";
// For Xbee, it is recommended that ip address be used
const char *MQTT_BROKER = "192.168.0.100";

DigiXbeeS6B digixbee(XbeeSerial, XBEE_PWR);
DigiXbeeS6B &modem = digixbee;

HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

const char *datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";
Observation temperature;

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

  // if you are connecting Xbee to your wifi network for the first time, give
  // more time for connection
  if (!modem.connectToInternet(wifiId, wifiPwd)) {
    Serial.println("Wifi is not connected");
    while (1)
      ;
    // if you think, your wifi credential is correct, let the code run for some
    // minutes and restart the program wifi credential if correct will only
    // result in failure if the setup time is more than 120 seconds
  }
  Serial.println("Wifi is connected");
  delay(2000);
  // uint32_t datetime = modem.getNISTTime();
  // Serial.println(datetime);
  // setupDateTimeFromServer(datetime);
  // Serial.println("datetime updated from server");

  modem.extraSetupForMQTT(MQTT_BROKER, 1883);

  Serial.println("extra setup for mqtt done");
  Serial.println("Initializing broker connection");
  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("mayflylogger");
  if (!mqttClient.connectToBroker()) {
    Serial.println("Cannot connect to Broker. Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    String response = modem.getModem().sendATGetString(GF("DL"));
    Serial.println(response);
    Serial.println("let's see what happens to DL");
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

  unsigned long now = millis();
  if (now - lastPublishTime >= publishInterval) {
    lastPublishTime = now;

    float randomTemp = random(20, 25);
    temperature.value = randomTemp;
    mqttClient.publishObservation(temperature, getISO8601Time());

    Serial.println("published");
  }
}
