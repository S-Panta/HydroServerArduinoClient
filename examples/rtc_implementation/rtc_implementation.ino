#include <WiFiS3.h>

const char *ssid = "USU-guest";
const char *password = 0;
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "RTC.h"
WiFiUDP ntpUDP;

// 60 millisecond for update frequency
// setUpdateInterval can be used to change later
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", -6 * 3600, 60000);

String getISO8601Time(RTCTime &t) {
  char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t.getYear(),
           Month2int(t.getMonth()), t.getDayOfMonth(), t.getHour(),
           t.getMinutes(), t.getSeconds());
  return String(buf);
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
}

void loop() {
  mqttClient.poll();
  RTCTime currentTime;
  RTC.getTime(currentTime);
  Serial.println("publishing measurement");

  float temperature;
  temperature = random(20, 30);
  streamTemperature.value = temperature;

  mqttClient.publishObservation(streamTemperature, getISO8601Time(currentTime));
  delay(10000);
}
