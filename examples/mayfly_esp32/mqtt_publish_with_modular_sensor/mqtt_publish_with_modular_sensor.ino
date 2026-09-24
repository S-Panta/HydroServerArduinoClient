#include <Arduino.h>
#include <ModularSensors.h>

#define XbeeSerial Serial1
#define XBEE_PWR 18

#include "Sodaq_DS3231.h"
#include <HydroServerMQTTClient.h>
#include <modems/ExpressifESP32.h>

const char* sketchName = "combined_logging_mqtt.ino";
const char* LoggerID = "YourLoggerID";
const int8_t loggingInterval = 1;  
const int8_t timeZone = 0;          

const int32_t serialBaud = 115200;
const int8_t  greenLED   = 8;
const int8_t  redLED     = 9;
const int8_t  buttonPin  = 21;
const int8_t  wakePin    = 31;
const int8_t  sdCardPwrPin   = -1;
const int8_t  sdCardSSPin    = 12;
const int8_t  sensorPowerPin = 22;


const char *wifiId  = "USU-guest";
const char *wifiPwd = 0;
const char *MQTT_BROKER = "144.39.51.28";
const int32_t modemBaud = 57600;  // ESP32 requires 57600

ExpressifESP32  esp32(XbeeSerial, XBEE_PWR);
ExpressifESP32& modem = esp32;
HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

Observation boardTempObs;
// From hydroserver instance
const char* tempDataStreamId = "01a0025b-8101-7b64-92f4-ccf26c3c3cdb";


#include <sensors/ProcessorStats.h>
const char*    mcuBoardVersion = "v1.1";
ProcessorStats mcuBoard(mcuBoardVersion);

#include <sensors/MaximDS3231.h>
MaximDS3231 ds3231(1);


Variable* variableList[] = {
    new ProcessorStats_SampleNumber(&mcuBoard),  
    new MaximDS3231_Temp(&ds3231,tempDataStreamId)               
};

int variableCount = sizeof(variableList) / sizeof(variableList[0]);
VariableArray varArray;
Logger        dataLogger;


void greenRedFlash(uint8_t numFlash = 4, uint8_t rate = 75) {
    for (uint8_t i = 0; i < numFlash; i++) {
        digitalWrite(greenLED, HIGH);
        digitalWrite(redLED, LOW);
        delay(rate);
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, HIGH);
        delay(rate);
    }
    digitalWrite(redLED, LOW);
}

void publishCycle() {
    String ts = Logger::formatDateTime_ISO8601(Logger::markedLocalUnixTime);

    boardTempObs.value = variableList[1]->getValue();
    bool boardTempOK = mqttClient.publishObservation(boardTempObs, ts.c_str());

    Serial.println(boardTempOK ? F("Published to MQTT broker")
                                : F("Publish cycle failed — will retry next interval"));
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
    greenRedFlash();

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

    boardTempObs.datastreamId     = tempDataStreamId;
    boardTempObs.sensorId         = "boardtemp-sensor-id";
    boardTempObs.observedProperty = "temperature_celsius";

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