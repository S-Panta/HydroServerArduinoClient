#ifndef SRC_SENSORS_CS451_H_
#define SRC_SENSORS_CS451_H_

// Include the library config before anything else
#include "ModSensorConfig.h"

// Include the debugging config
#include "ModSensorDebugConfig.h"

// Define the print label[s] for the debugger
#ifdef MS_CS451_DEBUG
#define MS_DEBUGGING_STD "CS451"
#endif
#ifdef MS_SDI12SENSORS_DEBUG_DEEP
#define MS_DEBUGGING_DEEP "SDI12Sensors"
#endif

// Include the debugger
#include "ModSensorDebugger.h"
// Undefine the debugger label[s]
#undef MS_DEBUGGING_STD
#undef MS_DEBUGGING_DEEP

// Include other in-library and external dependencies
#include "VariableBase.h"
#include "sensors/SDI12Sensors.h"

// The CS451 measures water level (from pressure) and temperature
#define CS451_NUM_VARIABLES 2
// No calculated variables; level is reported directly by the sensor (mm)
#define CS451_INC_CALC_VARIABLES 0

// The CS451 wakes up instantly upon power application/SDI-12 break signal.
#define CS451_WARM_UP_TIME_MS       6000   
#define CS451_STABILIZATION_TIME_MS 30      // No stabilization wait required
// Sensor configured to average 50 measurements (aXCONFIG2=50!)
#define CS451_MEASUREMENT_TIME_MS   60000
#define CS451_EXTRA_WAKE_TIME_MS    50

// Variable names are from the ODM2 controlled vocabulary.
// Gage height
#define CS451_PRESSURE_RESOLUTION   5
#define CS451_PRESSURE_VAR_NUM      0
#define CS451_PRESSURE_VAR_NAME     "gageHeight"
#define CS451_PRESSURE_UNIT_NAME    "millimeter"
#define CS451_PRESSURE_DEFAULT_CODE "CS451GageHeight"

// Temperature
#define CS451_TEMP_RESOLUTION   3
#define CS451_TEMP_VAR_NUM      1
#define CS451_TEMP_VAR_NAME     "temperature"
#define CS451_TEMP_UNIT_NAME    "degreeCelsius"
#define CS451_TEMP_DEFAULT_CODE "CS451Temp"

class CS451 : public SDI12Sensors {
 public:
    /**
     * @brief Construct a new CS451 object.
     *
     * The SDI-12 address of the sensor, the Arduino pin controlling power
     * on/off, and the Arduino pin sending and receiving data are required for
     * the sensor constructor.  Optionally, you can include a number of distinct
     * readings to average.  The data pin must be a pin that supports pin-change
     * interrupts.
     *
     * @param SDI12address The SDI-12 address of the CS451; can be a char,
     * char*, or int. The CS451 ships from Campbell with address "0"; change
     * it to a unique address before using it with ModularSensors.
     * @param powerPin The pin on the mcu controlling power to the CS451.
     * Use -1 if it is continuously powered.
     * - The CS451 requires a 6-18V power supply, which can be turned off
     * between measurements.
     * @param dataPin The pin on the mcu connected to the data line of the
     * SDI-12 circuit.
     * @param measurementsToAverage The number of measurements to take and
     * average before giving a "final" result from the sensor; optional with a
     * default value of 1.
     */
    CS451(char SDI12address, int8_t powerPin, int8_t dataPin,
          uint8_t measurementsToAverage = 1)
        : SDI12Sensors(SDI12address, powerPin, dataPin, measurementsToAverage,
                       "CS451", CS451_NUM_VARIABLES, CS451_WARM_UP_TIME_MS,
                       CS451_STABILIZATION_TIME_MS, CS451_MEASUREMENT_TIME_MS,
                       CS451_EXTRA_WAKE_TIME_MS, CS451_INC_CALC_VARIABLES) {}
    CS451(char* SDI12address, int8_t powerPin, int8_t dataPin,
          uint8_t measurementsToAverage = 1)
        : SDI12Sensors(SDI12address, powerPin, dataPin, measurementsToAverage,
                       "CS451", CS451_NUM_VARIABLES, CS451_WARM_UP_TIME_MS,
                       CS451_STABILIZATION_TIME_MS, CS451_MEASUREMENT_TIME_MS,
                       CS451_EXTRA_WAKE_TIME_MS, CS451_INC_CALC_VARIABLES) {}
    CS451(int SDI12address, int8_t powerPin, int8_t dataPin,
          uint8_t measurementsToAverage = 1)
        : SDI12Sensors(SDI12address, powerPin, dataPin, measurementsToAverage,
                       "CS451", CS451_NUM_VARIABLES, CS451_WARM_UP_TIME_MS,
                       CS451_STABILIZATION_TIME_MS, CS451_MEASUREMENT_TIME_MS,
                       CS451_EXTRA_WAKE_TIME_MS, CS451_INC_CALC_VARIABLES) {}

    /**
     * @brief Destroy the CS451 object
     */
    ~CS451() override = default;

    bool addSingleMeasurementResult() override;

protected:
    int8_t startSDI12Measurement(bool isConcurrent = true);
};

/**
 * @brief The Variable sub-class used for the gage height output from a CS451.
 */
class CS451_GageHeight : public Variable {
 public:
    explicit CS451_GageHeight(CS451* parentSense, const char* uuid = "",
                              const char* varCode = CS451_PRESSURE_DEFAULT_CODE)
        : Variable(parentSense, (const uint8_t)CS451_PRESSURE_VAR_NUM,
                   (uint8_t)CS451_PRESSURE_RESOLUTION, CS451_PRESSURE_VAR_NAME,
                   CS451_PRESSURE_UNIT_NAME, varCode, uuid) {}
    CS451_GageHeight()
        : Variable((const uint8_t)CS451_PRESSURE_VAR_NUM,
                   (uint8_t)CS451_PRESSURE_RESOLUTION, CS451_PRESSURE_VAR_NAME,
                   CS451_PRESSURE_UNIT_NAME, CS451_PRESSURE_DEFAULT_CODE) {}
    ~CS451_GageHeight() {}
};

/**
 * @brief The Variable sub-class used for the temperature output from a CS451.
 */
class CS451_Temp : public Variable {
 public:
    explicit CS451_Temp(CS451* parentSense, const char* uuid = "",
                        const char* varCode = CS451_TEMP_DEFAULT_CODE)
        : Variable(parentSense, (const uint8_t)CS451_TEMP_VAR_NUM,
                   (uint8_t)CS451_TEMP_RESOLUTION, CS451_TEMP_VAR_NAME,
                   CS451_TEMP_UNIT_NAME, varCode, uuid) {}
    CS451_Temp()
        : Variable((const uint8_t)CS451_TEMP_VAR_NUM,
                   (uint8_t)CS451_TEMP_RESOLUTION, CS451_TEMP_VAR_NAME,
                   CS451_TEMP_UNIT_NAME, CS451_TEMP_DEFAULT_CODE) {}
    ~CS451_Temp() {}
};

#endif  // SRC_SENSORS_CS451_H_