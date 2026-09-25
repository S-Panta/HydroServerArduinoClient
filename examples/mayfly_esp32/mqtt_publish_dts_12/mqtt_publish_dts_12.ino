#include <Arduino.h>
#include <ModularSensors.h>

#define XbeeSerial Serial1
#define XBEE_PWR 18

#include "Sodaq_DS3231.h"
#include <HydroServerMQTTClient.h>
#include <modems/ExpressifESP32.h>

const char *sketchName = "mqtt_publish_dts_12.ino";
const char *LoggerID = "mayflylogger";
const int8_t loggingInterval = 1;
const int8_t timeZone = 0;

const int32_t serialBaud = 115200;
const int8_t greenLED = 8;
const int8_t redLED = 9;
const int8_t buttonPin = 21;
const int8_t wakePin = 31;
const int8_t sdCardPwrPin = -1;
const int8_t sdCardSSPin = 12;
const int8_t sensorPowerPin = 22;

const char *wifiId = "USU-guest";
const char *wifiPwd = 0;
const char *MQTT_BROKER = "144.39.51.28";
const int32_t modemBaud = 57600; // ESP32 requires 57600

ExpressifESP32 esp32(XbeeSerial, XBEE_PWR);
ExpressifESP32 &modem = esp32;
HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

#include <sensors/ProcessorStats.h>
const char *mcuBoardVersion = "v1.1";
ProcessorStats mcuBoard(mcuBoardVersion);

#include <sensors/MaximDS3231.h>
MaximDS3231 ds3231(1);

#include <sensors/DTS12.h>

const char DTS12SDI12address = '1';
const int8_t DTS12Power = sensorPowerPin;
const int8_t SDI12DataPin = 7;
const uint8_t DTS12NumberReadings = 1;

DTS12 dts12(DTS12SDI12address, DTS12Power, SDI12DataPin, DTS12NumberReadings);

Variable *variableList[] = {new ProcessorStats_SampleNumber(&mcuBoard),
                            new ProcessorStats_Battery(&mcuBoard),
                            new MaximDS3231_Temp(&ds3231),
                            new DTS12_Variance(&dts12),
                            new DTS12_Median_Turbidity(&dts12),
                            new DTS12_Temp(&dts12),
                            new DTS12_WipeStatus(&dts12)};

int variableCount = sizeof(variableList) / sizeof(variableList[0]);

VariableArray varArray;
Logger dataLogger;

Observation medianTurbidity;
Observation waterTemperature;
Observation varianceTurbidity;

void publishCycle() {
  String ts = Logger::formatDateTime_ISO8601(Logger::markedLocalUnixTime);

  medianTurbidity.value = variableList[4]->getValue();
  waterTemperature.value = variableList[5]->getValue();
  varianceTurbidity.value = variableList[3]->getValue();

  mqttClient.publishObservation(medianTurbidity, ts.c_str());
  mqttClient.publishObservation(waterTemperature, ts.c_str());
  mqttClient.publishObservation(varianceTurbidity, ts.c_str());
  Serial.println("message published to server");
}

void setup() {
  Serial.begin(serialBaud);
  delay(1000);

  Serial.print(F("Now running "));
  Serial.print(sketchName);
  Serial.print(F(" on Logger "));
  Serial.println(LoggerID);
  Serial.print(F("Using ModularSensors Library version "));
  Serial.println(MODULAR_SENSORS_VERSION);

  pinMode(greenLED, OUTPUT);
  digitalWrite(greenLED, LOW);
  pinMode(redLED, OUTPUT);
  digitalWrite(redLED, LOW);

  XbeeSerial.begin(modemBaud);
  modem.powerUp();
  delay(1000);
  Serial.println(F("Powered up modem"));

  if (!modem.connectToInternet(wifiId, wifiPwd)) {
    Serial.println(F("WiFi is not connected"));
  } else {
    Serial.println(F("WiFi is connected"));
    delay(2000);
    uint32_t unixTime = modem.getNISTTime();
    rtc.begin();
    rtc.setDateTime(unixTime);
    Serial.println(F("RTC synced from NIST time server"));
  }

  modem.extraSetupForMQTT();
  mqttClient.setSiteCode("uwrl");
  mqttClient.setClientID("MayFlyLogger");
  mqttClient.setLastWill("Mayfly Logger shutting down");
  mqttClient.setKeepAliveInterval(120);

  if (!mqttClient.connectToBroker()) {
    Serial.println("Cannot connect to Broker. Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
    // it make no sense to work further when connection to broker is not
    // successful
    while (1)
      ;
  };

  medianTurbidity.datastreamId = "median-turbidity";
  waterTemperature.datastreamId = "water-temperature";
  varianceTurbidity.datastreamId = "variance-water";

  medianTurbidity.sensorId = "DTS-12";
  waterTemperature.sensorId = "DTS-12";
  varianceTurbidity.sensorId = "DTS-12";

  medianTurbidity.observedProperty = "medianturbidity";
  waterTemperature.sensorId = "water_temp";
  varianceTurbidity.sensorId = "variance_in_turbidity";

  Logger::setLoggerTimeZone(timeZone);
  loggerClock::setRTCOffset(0);

  dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, buttonPin,
                           greenLED);

  varArray.begin(variableCount, variableList);
  dataLogger.begin(LoggerID, loggingInterval, &varArray);

  Serial.println(F("Setting up sensors..."));
  varArray.setupSensors();
  dataLogger.setFileName("testdata.csv");
  dataLogger.createLogFile(true);

  dataLogger.systemSleep();
}

void loop() {
  dataLogger.logData(false);
  publishCycle();
  mqttClient.poll();
  dataLogger.systemSleep();
}