#include <Arduino.h>
#include <ModularSensors.h>

#define XbeeSerial Serial1
#define XBEE_PWR 18

#include "Sodaq_DS3231.h"
#include <HydroServerMQTTClient.h>
#include <modems/ExpressifESP32.h>

const int8_t loggingInterval = 1;
const int8_t timeZone = 0;
const char *LoggerID = "mayfly";

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
const char *MQTT_BROKER = "raspberrypi1.mypc.usu.edu";
const int32_t modemBaud = 57600; // ESP32 requires 57600

ExpressifESP32 esp32(XbeeSerial, XBEE_PWR);
ExpressifESP32 &modem = esp32;
HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

#include <sensors/ProcessorStats.h>
const char *mcuBoardVersion = "v1.1";
ProcessorStats mcuBoard(mcuBoardVersion);

#include <sensors/MaximDS3231.h>
MaximDS3231 ds3231(1);

const int8_t SDI12DataPin = 7;

Variable *variableList[] = {
    new ProcessorStats_SampleNumber(&mcuBoard),
    new ProcessorStats_Battery(&mcuBoard),
    new MaximDS3231_Temp(&ds3231),
};

int variableCount = sizeof(variableList) / sizeof(variableList[0]);

VariableArray varArray;
Logger dataLogger;

Observation boardTempObs;
Observation batteryObs;
const char *batteryObsDataStreamid = "01a0025b-8101-7b64-92f4-ccf26c3c3cdc";
// From hydroserver instance; check in hydroserver
const char *tempDataStreamId = "019eae3f-3450-70db-b5d2-a55879b4d681";

void publishCycle() {
  String ts = Logger::formatDateTime_ISO8601(Logger::markedLocalUnixTime);

  batteryObs.value = variableList[1]->getValue();
  boardTempObs.value = variableList[2]->getValue();

  bool publishBattery = mqttClient.publishObservation(batteryObs, ts.c_str());
  bool publishTemp = mqttClient.publishObservation(boardTempObs, ts.c_str());

  bool publishOK = publishBattery && publishTemp;

  Serial.println(publishOK
                     ? F("Published to MQTT broker")
                     : F("Publish cycle failed — will retry next interval"));
}

void setup() {
  Serial.begin(serialBaud);
  delay(1000);

  Serial.print(F("Now running "));
  // __FILE__ prints full path so need to extract filename from that path
  Serial.println(__builtin_strrchr(__FILE__, '/') + 1);

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
  mqttClient.setKeepAliveInterval(660);

  if (!mqttClient.connectToBroker()) {
    Serial.println("Cannot connect to Broker. Connection Error is ");
    Serial.println(mqttClient.getConnectionError());
  } else {
    Serial.println("Connected to MQTT broker");
  };

  batteryObs.datastreamId = batteryObsDataStreamid;
  boardTempObs.datastreamId = tempDataStreamId;

  batteryObs.sensorId = "mayfly";
  boardTempObs.sensorId = "mayfly";

  batteryObs.observedProperty = "battery_voltage";
  boardTempObs.observedProperty = "board_temp";

  Logger::setLoggerTimeZone(timeZone);
  loggerClock::setRTCOffset(0);

  dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, buttonPin,
                           greenLED);

  varArray.begin(variableCount, variableList);
  // give back 5 measurement in interval of 1 minute for testing purpose
  dataLogger.setStartupMeasurements(5);
  dataLogger.begin(LoggerID, loggingInterval, &varArray);

  Serial.println(F("Setting up sensors..."));
  varArray.setupSensors();
  // dataLogger.setFileName("testdata.csv");
  // dataLogger.createLogFile(true);

  dataLogger.systemSleep();
}

void loop() {
  if (dataLogger.checkInterval()) {
    Serial.println(Logger::formatDateTime_ISO8601(rtc.now().getEpoch()));
    Serial.println("Taking measurement and sending to broker");
    varArray.completeUpdate();
    // dataLogger.logToSD();

    if (!mqttClient.isConnected()) {
      if (!modem.isInternetAvailable()) {
        modem.connectToInternet(wifiId, wifiPwd);
      }
      mqttClient.connectToBroker();
    }

    publishCycle();
  }
  mqttClient.poll();
  dataLogger.systemSleep();
}