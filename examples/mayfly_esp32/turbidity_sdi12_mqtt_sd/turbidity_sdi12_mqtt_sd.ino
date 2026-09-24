// ============================================================
// turbidity_sdi12_mqtt_sd.ino
//
// NOTE: kept at its original filename/folder so it stays the same file
// this project has been iterating on -- the content below no longer
// involves the DTS-12/turbidity sensor. Say the word if you'd like it
// renamed to something like rtc_temp_mqtt_sd.ino instead.
//
// Mayfly + EnviroDIY ESP32 WiFi Bee
//
// This version leans on the ModularSensors library (already installed on
// your system) for everything except the MQTT publish step:
//   - WiFi connection + RTC time sync   -> ModularSensors' EspressifESP32
//                                           modem + Logger::syncRTC()
//   - Reading the Mayfly's built-in DS3231 RTC temperature sensor
//                                       -> ModularSensors' MaximDS3231 /
//                                          MaximDS3231_Temp
//   - SD card logging                   -> ModularSensors' Logger
//                                          (createLogFile / logData)
//
// HydroServerMQTTClient (from this library) is imported directly and used
// right here to publish each reading to your MQTT broker -- no extra
// classes, no separate project.
//
// Every `loggingInterval` minutes this sketch:
//   1. Asks ModularSensors to read the DS3231 temperature and append a
//      row to the SD card log file (dataLogger.logData()).
//   2. Publishes that same reading over MQTT with HydroServerMQTTClient.
//
// REQUIRED LIBRARIES:
//   - HydroServerArduinoClient (this library)
//   - EnviroDIY_ModularSensors  (pulls in ArduinoJson, the EnviroDIY
//     TinyGsm fork, EnviroDIY_DS3231, SdFat, StreamDebugger, etc. as its
//     own dependencies -- nothing new to install beyond what you already
//     have for ModularSensors)
//   - ArduinoMqttClient (already listed as a HydroServerArduinoClient
//     dependency in library.properties)
//
// SETUP:
//   - Fill in your WiFi credentials + MQTT broker in arduino_secrets.h
//   - Replace the placeholder datastream UUID below with your real one
// ============================================================

#include <Arduino.h>
#include <HydroServerMQTTClient.h>
#include <ModularSensors.h>
#include "arduino_secrets.h"

// ---------------- Logging options ----------------
// Also used as the MQTT client ID -- lowercase is preferred, see
// HydroServerMQTTClient's docs/apireference.md
const char*  LoggerID        = "mayfly-temp-01";  // TODO: pick a unique ID
const int8_t loggingInterval = 2;                 // minutes
const int8_t timeZone        = 0;  // keep the RTC in UTC

const int32_t serialBaud   = 115200;
const int8_t  greenLED     = 8;
const int8_t  redLED       = 9;
const int8_t  buttonPin    = 21;
const int8_t  wakePin      = 31;  // Mayfly D31 = A7
const int8_t  sdCardPwrPin = -1;
const int8_t  sdCardSSPin  = 12;

// ---------------- ESP32 WiFi modem ----------------
// This is ModularSensors' OWN modem wrapper (a different class from this
// library's own ExpressifESP32) -- using it is what lets us call
// Logger::syncRTC() and Logger::createLogFile()/logData() below.
#include <modems/EspressifESP32.h>

HardwareSerial& modemSerial = Serial1;
const int32_t   modemBaud   = 115200;  // slowed down below if the Mayfly
                                       // can't keep up

const int8_t modemVccPin   = 18;
const int8_t modemResetPin = -1;
const int8_t modemLEDPin   = redLED;

const char* wifiId  = WIFI_SSID;
const char* wifiPwd = WIFI_PASS;

EspressifESP32 modemESP(&modemSerial, modemVccPin, modemResetPin, wifiId,
                        wifiPwd);
EspressifESP32 modem = modemESP;

// ---------------- Mayfly's built-in DS3231 RTC temperature sensor -------
#include <sensors/MaximDS3231.h>
MaximDS3231       ds3231(1);
MaximDS3231_Temp* ds3231Temp = new MaximDS3231_Temp(&ds3231);

// ---------------- Variable array + Logger (SD logging, RTC time) --------
Variable*     variableList[] = {ds3231Temp};
int           variableCount  = sizeof(variableList) / sizeof(variableList[0]);
VariableArray varArray(variableCount, variableList);
Logger        dataLogger(LoggerID, loggingInterval, &varArray);

// ---------------- MQTT (HydroServerMQTTClient, used directly) -----------
const char* mqttBrokerAddr = MQTT_BROKER;  // from arduino_secrets.h
const char* siteCode       = "uwrl";       // TODO: replace with your site code

// Created in setup() once the modem has a Client available
HydroServerMQTTClient* mqttClient = nullptr;

// TODO: replace with your real HydroServer datastream UUID
Observation temperature = {"temperature", "REPLACE_WITH_YOUR_DATASTREAM_UUID",
                           "mayfly-ds3231-temp"};

unsigned long lastLogTime = 0;

// ---------------- Working functions --------------------------------------
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

// ==========================================================================
void setup() {
    Serial.begin(serialBaud);
    delay(1000);
    Serial.print(F("Running "));
    Serial.println(__builtin_strrchr(__FILE__, '/') + 1);
    Serial.print(F("Using ModularSensors Library version "));
    Serial.println(MODULAR_SENSORS_VERSION);

    modemSerial.begin(modemBaud);

    pinMode(greenLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    pinMode(redLED, OUTPUT);
    digitalWrite(redLED, LOW);
    greenRedFlash();

    pinMode(20, OUTPUT);  // onboard flash chip select, Mayfly v1.0+

    // --- Time sync (ModularSensors) ---
    Logger::setLoggerTimeZone(timeZone);
    loggerClock::setRTCOffset(0);

    dataLogger.attachModem(modem);
    modem.setModemLED(modemLEDPin);
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, buttonPin,
                             greenLED);
    dataLogger.begin();

    // Verify communication with, and set up, the modem
    for (int8_t ntries = 5; ntries; ntries--) {
        if (modem.modemWake()) break;
        // if that didn't work, try changing baud rate
        modemSerial.begin(115200);
        modem.gsmModem.sendAT(GF("+UART_DEF=9600,8,1,0,0"));
        modem.gsmModem.waitResponse();
        modemSerial.end();
        modemSerial.begin(9600);
    }

    // Sync the RTC over NIST time (this wakes/connects/disconnects the
    // modem on its own)
    Serial.println(F("Syncing RTC..."));
    dataLogger.syncRTC();

    // --- SD card (ModularSensors) ---
    Serial.println(F("Setting up file on SD card"));
    dataLogger.turnOnSDcard(true);
    dataLogger.createLogFile(true);  // true = write a new header
    dataLogger.turnOffSDcard(true);

    // --- Reconnect to WiFi and hold the connection for MQTT ---
    Serial.println(F("Connecting to WiFi for MQTT..."));
    if (!modem.modemWake() || !modem.connectInternet()) {
        Serial.println(F("WiFi is not connected."));
    } else {
        Serial.println(F("WiFi is connected."));
    }

    // --- MQTT (HydroServerMQTTClient) ---
    Client* netClient = modem.createClient();
    mqttClient         = new HydroServerMQTTClient(*netClient, mqttBrokerAddr);
    mqttClient->setSiteCode(siteCode);
    mqttClient->setClientID(LoggerID);
    Serial.println(F("Connecting to MQTT broker..."));
    if (!mqttClient->connectToBroker()) {
        Serial.println(F("Connection to broker failed. Connection error:"));
        Serial.println(mqttClient->getConnectionError());
    } else {
        Serial.println(F("Connected to MQTT broker."));
    }

    // take a reading right away instead of waiting a full interval
    lastLogTime = millis() - (uint32_t)loggingInterval * 60000UL;
}

// ==========================================================================
void loop() {
    if (mqttClient != nullptr) { mqttClient->poll(); }

    unsigned long now = millis();
    if (now - lastLogTime >= (uint32_t)loggingInterval * 60000UL) {
        lastLogTime = now;

        // --- Read the sensor + log to SD (ModularSensors) ---
        Serial.println(F("Logging..."));
        dataLogger.logData();  // reads ds3231Temp and appends a row to the
                               // SD card log file

        float tempC = ds3231Temp->getValue();
        Serial.print(F("Mayfly RTC temperature: "));
        Serial.print(tempC);
        Serial.println(F(" C"));

        // --- Publish over MQTT (HydroServerMQTTClient) ---
        if (mqttClient != nullptr) {
            String isoTime =
                Logger::formatDateTime_ISO8601(Logger::markedLocalUnixTime);

            temperature.value = tempC;
            mqttClient->publishObservation(temperature, isoTime.c_str());
            Serial.println(mqttClient->getObservationTopic(temperature));
        }
    }
}
