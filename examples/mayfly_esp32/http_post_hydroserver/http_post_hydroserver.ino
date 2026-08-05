#define XbeeSerial Serial1
#define XBEE_PWR 18

#include "Sodaq_DS3231.h"
#include "arduino_secrets.h"
#include <HydroServerHTTPClient.h>
#include <modems/ExpressifESP32.h>

// wifi details
const char *wifiId = "USU-guest";
const char *wifiPwd = 0;

const char *serverAddress = "144.39.67.171";
const int serverPort = 80;
const char *apiKey = LOCAL_API_KEY;
const char *datastreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";

const long interval = 15000;
unsigned long previousMillis = 0;

ExpressifESP32 esp32(XbeeSerial, XBEE_PWR);
ExpressifESP32 &modem = esp32;

HydroServerHTTPClient hsClient(*modem.createClient(), serverAddress,
                               serverPort);
Observation temperature;

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
  Serial.print("Running sketch ");
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);

  XbeeSerial.begin(57600);
  delay(1000);

  modem.powerUp();
  delay(1000);
  Serial.println("Xbee module powered up");

  if (!modem.connectToInternet(wifiId, wifiPwd)) {
    Serial.println("Wifi is not connected");
  }
  Serial.println("Wifi is connected");
  delay(2000);

  Serial.println("setting time from server");
  uint32_t datetime = modem.getNISTTime();
  setupDateTimeFromServer(datetime);
  Serial.println("Time setup from server completed");

  Serial.println("running extra setup for http");
  modem.extraSetupForHTTPS(serverAddress, serverPort);
  Serial.println("extra setup for https completed");

  hsClient.setApiKey(apiKey);

  temperature.datastreamId = datastreamId;
  temperature.sensorId = "sensorid";
  temperature.observedProperty = "Temperature";
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Serial.println("posting to hydroserver");
    double randomValue = random(3500, 5000) / 100.0;
    temperature.value = randomValue;
    int status = hsClient.publishObservation(temperature, getISO8601Time());
    Serial.println(status);
    Serial.println(hsClient.getStatusCode());
    // Serial.print(hsClient.getResponseBody());
    Serial.println(randomValue);
    Serial.println("..................................................");
  }
}
