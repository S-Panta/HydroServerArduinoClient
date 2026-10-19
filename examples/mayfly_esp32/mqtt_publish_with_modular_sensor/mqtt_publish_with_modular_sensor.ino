// ============================================================
// mqtt_publish_with_modular_sensor.ino
//
// EnviroDIY Mayfly + ESP32 Wi-Fi Bee + ModularSensors: every logging
// interval, wake up, read the Mayfly's battery voltage and board
// temperature, publish them over MQTT, then go back to sleep.
//
// Hardware:  EnviroDIY Mayfly v1.x, ESP32 Bee (Espressif AT firmware)
// Libraries: HydroServerArduinoClient, ArduinoMqttClient, ArduinoJson,
//            EnviroDIY TinyGSM fork, StreamDebugger,
//            EnviroDIY ModularSensors (and its dependencies)
//
// Before uploading:
//   1. Create arduino_secrets.h next to this sketch with
//        #define WIFI_SSID "your-network"
//        #define WIFI_PASS "your-password"   // use 0 for an open network
//   2. Set MQTT_BROKER, SITE_CODE, LOGGER_ID and the datastream IDs below.
// ============================================================

// Include ModularSensors before the other libraries
#include <ModularSensors.h>

#include "arduino_secrets.h"
#include <HydroServerMQTTClient.h>
#include <Sodaq_DS3231.h>
#include <modems/EspressifESP32.h>

// ---- Configuration -------------------------------------------------------
const char *MQTT_BROKER = "test.mosquitto.org";
const char *SITE_CODE = "uwrl";
// Used as the ModularSensors logger ID and as the MQTT client ID
const char *LOGGER_ID = "mayfly-esp32-01";

// Minutes between readings. 1 is handy for testing; use a longer interval
// in the field.
const int8_t LOGGING_INTERVAL_MINUTES = 1;
const int8_t TIME_ZONE = 0; // keep the logger in UTC

// ---- Mayfly pins ---------------------------------------------------------
const int32_t SERIAL_BAUD = 115200;
const int8_t GREEN_LED = 8;
const int8_t RED_LED = 9;
const int8_t BUTTON_PIN = 21;
const int8_t WAKE_PIN = 31;
const int8_t SD_CARD_POWER_PIN = -1;
const int8_t SD_CARD_SS_PIN = 12;

// ---- Modem ---------------------------------------------------------------
HardwareSerial &beeSerial = Serial1; // Bee socket serial port
const int8_t BEE_POWER_PIN = 18;     // Bee socket power pin on Mayfly v1.x
const uint32_t BEE_BAUD = 57600;     // ESP32 AT firmware baud rate

EspressifESP32 modem(beeSerial, BEE_POWER_PIN);
HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);

// ---- Sensors -------------------------------------------------------------
#include <sensors/ProcessorStats.h>
ProcessorStats mcuBoard("v1.1"); // Mayfly board version

#include <sensors/MaximDS3231.h>
MaximDS3231 ds3231(1);

// Keep a named pointer to each variable you publish, so the code below does
// not depend on its position in variableList.
Variable *batteryVoltage = new ProcessorStats_Battery(&mcuBoard);
Variable *boardTemperature = new MaximDS3231_Temp(&ds3231);

Variable *variableList[] = {
    new ProcessorStats_SampleNumber(&mcuBoard),
    batteryVoltage,
    boardTemperature,
};
const int VARIABLE_COUNT = sizeof(variableList) / sizeof(variableList[0]);

VariableArray varArray;
Logger dataLogger;

// ---- Observations --------------------------------------------------------
// observedProperty, datastreamId (from HydroServer), sensorId
Observation batteryObs = {"battery_voltage",
                          "01a0025b-8101-7b64-92f4-ccf26c3c3cdc", "mayfly"};
Observation boardTempObs = {"board_temperature",
                            "019eae3f-3450-70db-b5d2-a55879b4d681", "mayfly"};

Observation *observations[] = {&batteryObs, &boardTempObs};
const uint8_t OBSERVATION_COUNT =
    sizeof(observations) / sizeof(observations[0]);

// ---- Helpers -------------------------------------------------------------
// Reconnects Wi-Fi and the broker if either dropped while asleep.
void ensureConnected() {
  if (mqttClient.isConnected()) {
    return;
  }
  Serial.println(F("MQTT disconnected, reconnecting"));
  if (!modem.isInternetAvailable()) {
    Serial.println(modem.connectToInternet(WIFI_SSID, WIFI_PASS)
                       ? F("Wi-Fi reconnected")
                       : F("Wi-Fi reconnect failed"));
  }
  if (!mqttClient.connectToBroker()) {
    Serial.print(F("MQTT connection failed, error code: "));
    Serial.println(mqttClient.getConnectionError());
  }
}

void publishCycle() {
  // Timestamp of this logging interval, in ISO 8601 UTC
  String timestamp =
      Logger::formatDateTime_ISO8601(Logger::markedLocalUnixTime);

  batteryObs.value = batteryVoltage->getValue();
  boardTempObs.value = boardTemperature->getValue();

  uint8_t published =
      mqttClient.publishAll(observations, OBSERVATION_COUNT, timestamp.c_str());
  if (published == OBSERVATION_COUNT) {
    Serial.println(F("Published to MQTT broker"));
  } else {
    Serial.println(F("Publish failed, will retry next interval"));
  }
}

// ---- Sketch --------------------------------------------------------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.print(F("Running on logger "));
  Serial.println(LOGGER_ID);

  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);

  beeSerial.begin(BEE_BAUD);
  modem.powerUp();
  delay(1000); // give the ESP32 time to boot

  if (modem.connectToInternet(WIFI_SSID, WIFI_PASS)) {
    Serial.println(F("Wi-Fi connected"));
    // getNISTTime() returns seconds since 2000-01-01, the DS3231's epoch
    rtc.begin();
    rtc.setDateTime(modem.getNISTTime());
    Serial.println(F("Clock set from NTP"));
  } else {
    Serial.println(F("Wi-Fi connection failed, will retry each interval"));
  }

  // Required on the ESP32 before opening an MQTT connection
  modem.extraSetupForMQTT();

  mqttClient.setSiteCode(SITE_CODE);
  mqttClient.setClientID(LOGGER_ID);
  mqttClient.setLastWill("offline");
  // Longer than the logging interval, so the broker doesn't drop the
  // connection while the logger sleeps
  mqttClient.setKeepAliveInterval(LOGGING_INTERVAL_MINUTES * 60 + 60);

  if (mqttClient.connectToBroker()) {
    Serial.println(F("Connected to MQTT broker"));
  } else {
    Serial.print(F("MQTT connection failed, error code: "));
    Serial.println(mqttClient.getConnectionError());
  }

  Logger::setLoggerTimeZone(TIME_ZONE);
  loggerClock::setRTCOffset(0);
  dataLogger.setLoggerPins(WAKE_PIN, SD_CARD_SS_PIN, SD_CARD_POWER_PIN,
                           BUTTON_PIN, GREEN_LED);

  varArray.begin(VARIABLE_COUNT, variableList);
  // Take 5 readings one minute apart right after start-up (useful when
  // testing); set to 0 in the field
  dataLogger.setStartupMeasurements(5);
  dataLogger.begin(LOGGER_ID, LOGGING_INTERVAL_MINUTES, &varArray);

  Serial.println(F("Setting up sensors"));
  varArray.setupSensors();

  dataLogger.systemSleep();
}

void loop() {
  if (dataLogger.checkInterval()) {
    Serial.println(F("Taking measurements"));
    varArray.completeUpdate();

    ensureConnected();
    publishCycle();
  }
  mqttClient.poll();
  dataLogger.systemSleep();
}
