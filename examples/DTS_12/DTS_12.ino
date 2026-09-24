#include <Arduino.h>
#include <ModularSensors.h>


const char* sketchName      = "DTS12_logger.ino";
const char* LoggerID        = "DTS12_Logger";
const int8_t loggingInterval = 1;   
const int8_t timeZone        = 0;   

const int32_t serialBaud     = 115200;
const int8_t  greenLED       = 8;
const int8_t  redLED         = 9;
const int8_t  buttonPin      = 21;
const int8_t  wakePin        = 31;
const int8_t  sdCardPwrPin   = -1;
const int8_t  sdCardSSPin    = 12;
const int8_t  sensorPowerPin = 22;


#include <sensors/ProcessorStats.h>
const char*    mcuBoardVersion = "v1.1";
ProcessorStats mcuBoard(mcuBoardVersion);

#include <sensors/MaximDS3231.h>
MaximDS3231 ds3231(1);

#define MS_SDI12SENSORS_DEBUG_DEEP
#include <sensors/DTS12.h> 

const char    DTS12SDI12address   = '1';  
const int8_t  DTS12Power          = sensorPowerPin; 
const int8_t  SDI12DataPin        = 7;
const uint8_t DTS12NumberReadings = 1;

DTS12 dts12(DTS12SDI12address, DTS12Power, SDI12DataPin, DTS12NumberReadings);


Variable* variableList[] = {
    new ProcessorStats_SampleNumber(&mcuBoard),
    new ProcessorStats_Battery(&mcuBoard),
    new MaximDS3231_Temp(&ds3231),
    new DTS12_Variance(&dts12),
    new DTS12_Median_Turbidity(&dts12),
    new DTS12_Temp(&dts12),
    new DTS12_WipeStatus(&dts12)
};

int variableCount = sizeof(variableList) / sizeof(variableList[0]);

VariableArray varArray;
Logger        dataLogger;


void setup() {
    Serial.begin(serialBaud);
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

    Logger::setLoggerTimeZone(timeZone);
    loggerClock::setRTCOffset(0);

    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, buttonPin,
                             greenLED);

    varArray.begin(variableCount, variableList);
    dataLogger.begin(LoggerID, loggingInterval, &varArray);

    // This also enables the pin-change interrupt on the SDI-12 data pin,
    // which mySDI12 needs, so run the direct test after it.
    Serial.println(F("Setting up sensors..."));
    varArray.setupSensors();
    // dataLogger.createLogFile(true);
    Serial.println("look below");
    Serial.println(dts12.getSensorSerialNumber());
    Serial.println(dts12.getSensorNameAndLocation());
}

void loop() {
    // Only measure when the RTC hits a logging interval
    if (dataLogger.checkInterval()) {
        dataLogger.alertOn();                 // green LED on while measuring

        varArray.completeUpdate();            // power up, wake, measure, power down

        // Show the values on the Serial Monitor
        Serial.println(F("------ New reading ------"));
        varArray.printSensorData(&Serial);
        Serial.flush();                       // let it finish printing before sleep

        dataLogger.logToSD();                 // append the row to the CSV
        dataLogger.alertOff();
    }

    dataLogger.systemSleep();                 // sleep until the next RTC alarm
}