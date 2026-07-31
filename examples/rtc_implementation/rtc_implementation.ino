#include "arduino_secrets.h"
#include <WiFiS3.h>

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;
#include <NTPClient.h>
#include <WiFiUdp.h>

const long interval = 10000;
unsigned long previousMillis = 0;

#include "RTC.h"
WiFiUDP ntpUDP;

const char *MQTT_BROKER = "raspberrypi1.mypc.usu.edu";

#include <HydroServerMQTTClient.h>
WiFiClient wifiClient;
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);

Observation temperature = {"temperature", "uuidtemperature", "tempsensorid"};
Observation ph = {"pH", "uuidPh", "phsensorid"};

Observation *observations[] = {&temperature, &ph};

NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org");

char *getISO8601Time() {
  RTCTime t;
  RTC.getTime(t);
  static char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t.getYear(),
           Month2int(t.getMonth()), t.getDayOfMonth(), t.getHour(),
           t.getMinutes(), t.getSeconds());
  return buf;
}

void connectWiFi() {
  delay(2000);
  Serial.print("Connecting to WiFi: ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  delay(5000);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void syncRTCFromNTP() {
  timeClient.update();
  unsigned long unixTime = timeClient.getEpochTime();
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);
  Serial.print("RTC set to: ");
  Serial.println(timeClient.getFormattedTime());
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("running rtc");
  connectWiFi();
  timeClient.update();
  RTC.begin();
  syncRTCFromNTP();

  Serial.println("connecting to broker");

  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("Arduinopublisher");

  if (!mqttClient.connectToBroker()) {
    Serial.println("connection not successful.Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    while (1)
      ;
  };
}

void loop() {
  mqttClient.poll();

  Serial.print("loop runs  every 10 seconds");
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    Serial.println("publishing every 10 second");

    float randomTemp, randomPh;
    randomTemp = random(200, 351) / 10.0;
    randomPh = random(1, 7);
    previousMillis = currentMillis;

    temperature.value = randomTemp;
    ph.value = randomPh;

    uint8_t size = sizeof(observations) / sizeof(observations[0]);

    mqttClient.publishAll(observations, size, getISO8601Time());
    Serial.println(temperature.value);
    Serial.println(ph.value);
  };
  delay(10000);
}