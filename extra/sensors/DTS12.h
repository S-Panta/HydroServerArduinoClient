#ifndef SRC_SENSORS_DTS12_H_
#define SRC_SENSORS_DTS12_H_

// Include the library config before anything else
#include "ModSensorConfig.h"

// Include the debugging config
#include "ModSensorDebugConfig.h"

// Define the print label[s] for the debugger
#ifdef MS_DTS12_DEBUG
#define MS_DEBUGGING_STD "DTS12"
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

// Default aM2!/aD0! response: Mean Turbidity, Variance, Median
// Turbidity,BES,Min of last 100 readings, Max of last 100 readings,Temperature,
// Wipe Status
#define DTS12_NUM_VARIABLES 7
#define DTS12_INC_CALC_VARIABLES 0

// When power is first applied to the DTS-12 a wipe cycle will occur which takes
// about 5 seconds.
#define DTS12_WARM_UP_TIME_MS 9000
#define DTS12_STABILIZATION_TIME_MS 0
// the measurement time depends upon the ACK ttt response from SDI 12 commands,
// check if there is any extra time delay
#define DTS12_MEASUREMENT_TIME_MS 40000
#define DTS12_EXTRA_WAKE_TIME_MS 0

// Variable name are always from ODM2 controlled vocabulary.
// Because all except temperature is turbidity measurement, the variable name
// will be turbidity. But their aggregation statistic will differ:
// http://vocabulary.odm2.org/aggregationstatistic/
#define DTS12_MEAN_VAR_NUM 0
#define DTS12_MEAN_RESOLUTION 2
#define DTS12_MEAN_VAR_NAME "turbidity"
#define DTS12_MEAN_UNIT_NAME "nephelometricTurbidityUnit"
#define DTS12_MEAN_DEFAULT_CODE "DTS12Mean"

#define DTS12_VARIANCE_VAR_NUM 1
#define DTS12_VARIANCE_RESOLUTION 2
#define DTS12_VARIANCE_VAR_NAME "turbidity"
#define DTS12_VARIANCE_UNIT_NAME "nephelometricTurbidityUnitSquared"
#define DTS12_VARIANCE_DEFAULT_CODE "DTS12Variance"

// 2 - Median
#define DTS12_TURB_VAR_NUM 2
#define DTS12_TURB_RESOLUTION 2
#define DTS12_TURB_VAR_NAME "turbidity"
#define DTS12_TURB_UNIT_NAME "nephelometricTurbidityUnit"
#define DTS12_TURB_DEFAULT_CODE "DTS12Median"

// BES stands for Best Easy Systematic estimator.
// See Section 1.7.2 DTS-12 Calculations to understand how BES is computed.
// See: https://www.statisticshowto.com/trimean/
#define DTS12_BES_VAR_NUM 3
#define DTS12_BES_RESOLUTION 2
#define DTS12_BES_VAR_NAME "turbidity"
#define DTS12_BES_UNIT_NAME "nephelometricTurbidityUnit"
#define DTS12_BES_DEFAULT_CODE "DTS12BES"

// 4 - Min of last 100 readings
#define DTS12_MIN_VAR_NUM 4
#define DTS12_MIN_RESOLUTION 2
#define DTS12_MIN_VAR_NAME "turbidity"
#define DTS12_MIN_UNIT_NAME "nephelometricTurbidityUnit"
#define DTS12_MIN_DEFAULT_CODE "DTS12Min"

// 5 - Max of last 100 readings
#define DTS12_MAX_VAR_NUM 5
#define DTS12_MAX_RESOLUTION 2
#define DTS12_MAX_VAR_NAME "turbidity"
#define DTS12_MAX_UNIT_NAME "nephelometricTurbidityUnit"
#define DTS12_MAX_DEFAULT_CODE "DTS12Max"

// Water temperature: // operating range 0C to +40C (non-freezing), accuracy
// +/-0.2C
#define DTS12_TEMP_VAR_NUM 6
#define DTS12_TEMP_RESOLUTION 2
#define DTS12_TEMP_VAR_NAME "temperature"
#define DTS12_TEMP_UNIT_NAME "degreeCelsius"
#define DTS12_TEMP_DEFAULT_CODE "DTS12Temp"

class DTS12 : public SDI12Sensors {
public:
  // "The DTS-12 is shipped with default address 0 (unless
  // shipped as part of an integrated FTS system)." Must be changed to a
  // unique address before use with mayfly logger.
  /**
   * @brief Construct a new DTS12 object.
   *
   * The SDI-12 address of the sensor, the Arduino pin controlling power
   * on/off, and the Arduino pin sending and receiving data are required for
   * the sensor constructor.  Optionally, you can include a number of distinct
   * readings to average.  The data pin must be a pin that supports pin-change
   * interrupts.
   *
   * @param SDI12address The SDI-12 address of the DTS12; can be a char,
   * char*, or int. The DTS-12 ships with address "0" (unless part of an
   * integrated FTS system); change it to a unique address before using it
   * with ModularSensors.
   * @param powerPin The pin on the mcu controlling power to the DTS12.
   * Use -1 if it is continuously powered.
   * - The DTS12 requires a ~12V supply (check the FTS manual for the exact
   * range), which can be turned off between measurements. Note that a wipe
   * cycle runs each time power is applied.
   * @param dataPin The pin on the mcu connected to the data line of the
   * SDI-12 circuit.
   * @param measurementsToAverage The number of measurements to take and
   * average before giving a "final" result from the sensor; optional with a
   * default value of 1.
   */
  DTS12(char SDI12address, int8_t powerPin, int8_t dataPin,
        uint8_t measurementsToAverage = 1)
      : SDI12Sensors(SDI12address, powerPin, dataPin, measurementsToAverage,
                     "DTS12", DTS12_NUM_VARIABLES, DTS12_WARM_UP_TIME_MS,
                     DTS12_STABILIZATION_TIME_MS, DTS12_MEASUREMENT_TIME_MS,
                     DTS12_EXTRA_WAKE_TIME_MS, DTS12_INC_CALC_VARIABLES) {}
  DTS12(char *SDI12address, int8_t powerPin, int8_t dataPin,
        uint8_t measurementsToAverage = 1)
      : SDI12Sensors(SDI12address, powerPin, dataPin, measurementsToAverage,
                     "DTS12", DTS12_NUM_VARIABLES, DTS12_WARM_UP_TIME_MS,
                     DTS12_STABILIZATION_TIME_MS, DTS12_MEASUREMENT_TIME_MS,
                     DTS12_EXTRA_WAKE_TIME_MS, DTS12_INC_CALC_VARIABLES) {}
  DTS12(int SDI12address, int8_t powerPin, int8_t dataPin,
        uint8_t measurementsToAverage = 1)
      : SDI12Sensors(SDI12address, powerPin, dataPin, measurementsToAverage,
                     "DTS12", DTS12_NUM_VARIABLES, DTS12_WARM_UP_TIME_MS,
                     DTS12_STABILIZATION_TIME_MS, DTS12_MEASUREMENT_TIME_MS,
                     DTS12_EXTRA_WAKE_TIME_MS, DTS12_INC_CALC_VARIABLES) {}

  // destroy the sensor object
  ~DTS12() override = default;

  bool addSingleMeasurementResult() override;

protected:
  int8_t startSDI12Measurement(bool isConcurrent = true);
};

class DTS12_Mean_Turbidity : public Variable {
public:
  explicit DTS12_Mean_Turbidity(DTS12 *parentSense, const char *uuid = "",
                                const char *varCode = DTS12_MEAN_DEFAULT_CODE)
      : Variable(parentSense, DTS12_MEAN_VAR_NUM, DTS12_MEAN_RESOLUTION,
                 DTS12_MEAN_VAR_NAME, DTS12_MEAN_UNIT_NAME, varCode, uuid) {}
  ~DTS12_Mean_Turbidity() override = default;
};

class DTS12_Variance : public Variable {
public:
  explicit DTS12_Variance(DTS12 *parentSense, const char *uuid = "",
                          const char *varCode = DTS12_VARIANCE_DEFAULT_CODE)
      : Variable(parentSense, DTS12_VARIANCE_VAR_NUM, DTS12_VARIANCE_RESOLUTION,
                 DTS12_VARIANCE_VAR_NAME, DTS12_VARIANCE_UNIT_NAME, varCode,
                 uuid) {}
  ~DTS12_Variance() override = default;
};

class DTS12_Median_Turbidity : public Variable {
public:
  explicit DTS12_Median_Turbidity(DTS12 *parentSense, const char *uuid = "",
                                  const char *varCode = DTS12_TURB_DEFAULT_CODE)
      : Variable(parentSense, DTS12_TURB_VAR_NUM, DTS12_TURB_RESOLUTION,
                 DTS12_TURB_VAR_NAME, DTS12_TURB_UNIT_NAME, varCode, uuid) {}
  ~DTS12_Median_Turbidity() override = default;
};

class DTS12_BES_Turbidity : public Variable {
public:
  explicit DTS12_BES_Turbidity(DTS12 *parentSense, const char *uuid = "",
                               const char *varCode = DTS12_BES_DEFAULT_CODE)
      : Variable(parentSense, DTS12_BES_VAR_NUM, DTS12_BES_RESOLUTION,
                 DTS12_BES_VAR_NAME, DTS12_BES_UNIT_NAME, varCode, uuid) {}
  ~DTS12_BES_Turbidity() override = default;
};

class DTS12_Min_Turbidity : public Variable {
public:
  explicit DTS12_Min_Turbidity(DTS12 *parentSense, const char *uuid = "",
                               const char *varCode = DTS12_MIN_DEFAULT_CODE)
      : Variable(parentSense, DTS12_MIN_VAR_NUM, DTS12_MIN_RESOLUTION,
                 DTS12_MIN_VAR_NAME, DTS12_MIN_UNIT_NAME, varCode, uuid) {}
  ~DTS12_Min_Turbidity() override = default;
};

class DTS12_Max_Turbidity : public Variable {
public:
  explicit DTS12_Max_Turbidity(DTS12 *parentSense, const char *uuid = "",
                               const char *varCode = DTS12_MAX_DEFAULT_CODE)
      : Variable(parentSense, DTS12_MAX_VAR_NUM, DTS12_MAX_RESOLUTION,
                 DTS12_MAX_VAR_NAME, DTS12_MAX_UNIT_NAME, varCode, uuid) {}
  ~DTS12_Max_Turbidity() override = default;
};

class DTS12_Temp : public Variable {
public:
  explicit DTS12_Temp(DTS12 *parentSense, const char *uuid = "",
                      const char *varCode = DTS12_TEMP_DEFAULT_CODE)
      : Variable(parentSense, DTS12_TEMP_VAR_NUM, DTS12_TEMP_RESOLUTION,
                 DTS12_TEMP_VAR_NAME, DTS12_TEMP_UNIT_NAME, varCode, uuid) {}
  ~DTS12_Temp() override = default;
};

#endif
