#define LIBCALL_ENABLEINTERRUPT
#include "ModSensorInterrupts.h"

#include "CS451.h"

#ifdef MS_SDI12_NO_CRC_CHECK
#define MS_SDI12_USE_CRC false
#else
#define MS_SDI12_USE_CRC true
#endif

// This overrides the method in SDI12Sensor class
// We need to send aM8! command to get all average of 50 measurement of
// pressure/level All the measurement logic are from modularsensor
// SDI12Sensors::startSDI12Measurement()
int8_t CS451::startSDI12Measurement(bool isConcurrent) {
  String startCommand;
  String sdiResponse;
  String returnedAddress;

  // Try up to 5 times to start a measurement
  uint8_t numVariables = 0;
  uint8_t ntries = 0;
  bool didAcknowledge = false;
  int8_t wait = -1; // NOTE: The wait time can be 0!
  while (!didAcknowledge && ntries < 5) {
    if (isConcurrent) {
      MS_DBG(F("  Beginning concurrent measurement on"),
             getSensorNameAndLocation());
    } else {
      MS_DBG(F("  Beginning NON-concurrent (standard) measurement on"),
             getSensorNameAndLocation());
    }
    startCommand = "";
    startCommand += _SDI12address;
    if (isConcurrent) {
      startCommand += "C"; // Start concurrent measurement - 'C'
    } else {
      startCommand += "M"; // Start standard measurement - 'M'
    }
    if (MS_SDI12_USE_CRC) {
      startCommand += "C"; // Add C to request a CRC
    }
    startCommand += "8"; // Adding 1 after M gives 7 variable
    startCommand += "!"; // All commands end with '!'
    _SDI12Internal.clearBuffer();
    _SDI12Internal.sendCommand(startCommand, _extraWakeTime);
    delay(30); // It just needs this little delay
    MS_DEEP_DBG(F("    >>>"), startCommand);

    // wait for acknowledgement with format
    // [address][ttt (3 char, seconds)][number of values to be returned,
    // 0-9]<CR><LF>
    sdiResponse = _SDI12Internal.readStringUntil('\n');
    sdiResponse.trim();
    _SDI12Internal.clearBuffer();
    MS_DEEP_DBG(F("    <<<"), sdiResponse);

    // find out how long we have to wait (in seconds).
    if (sdiResponse.length() > 3) {
      returnedAddress = sdiResponse.substring(0, 1);
      wait = static_cast<uint8_t>(sdiResponse.substring(1, 4).toInt());
      numVariables = static_cast<uint8_t>(sdiResponse.substring(4).toInt());
    }
    MS_DEEP_DBG(F("   Responding address:"), returnedAddress, F("wait time:"),
                wait, F("result count:"), numVariables);
    // Only require that the responding address be correct to consider the
    // result to have been started
    if (returnedAddress == String(_SDI12address)) {
      didAcknowledge = true;
    } else {
      // print a warning if the responding address is wrong (and try
      // again)
      MS_DBG(F("   Wrong address replied, got"), returnedAddress,
             F("instead of"), _SDI12address);
    }
    // Print a warning if the wait is going to be longer than we expect
    if (wait > ceil(_measurementTime_ms / 1000)) {
      MS_DBG(F("   Wait time is too long"), wait * 1000, F("instead of"),
             _measurementTime_ms);
    }
    // Print a warning if the number of returned results is wrong
    if (numVariables != _numReturnedValues) {
      MS_DBG(F("   Wrong number of results expected"), wait * 1000,
             F("instead of"), (_numReturnedValues - _incCalcValues));
    }

    // Empty the buffer again
    _SDI12Internal.clearBuffer();
    ntries++;
  }

  // Return how long we're expecting to wait for a measurement
  // NOTE:  The sensor generally returns a value rounded up to the next
  // second.
  return wait;
}

// The function the logger calls to collect the result.
bool CS451::addSingleMeasurementResult() {
  // Perform common initialization checks
  if (!initializeMeasurementResult()) {
    return false;
  }

  bool success = false;

  String startCommand;
  String sdiResponse;

  // activate the SDI-12 object
  activate();

  // Check that the sensor is there and responding
  if (requestSensorAcknowledgement()) {
    // send the commands to start the measurement; false = not concurrent
    // the returned wait time should always be non-zero
    int8_t wait = startSDI12Measurement(false);

    // Set the times we've activated the sensor and asked for a measurement
    if (wait >= 0) {
      MS_DBG(F("    NON-concurrent measurement started."));
      // Update the time that a measurement was requested
      _millisMeasurementRequested = millis();
      // Re-set the status bit for measurement start success (bit 6)
      setStatusBit(MEASUREMENT_SUCCESSFUL);

      // Since this is not a concurrent measurement, we must sit around
      // and wait for the sensor to issue a service request telling us
      // that the measurement is ready.

      uint32_t timerStart = millis();
      while ((millis() - timerStart) < static_cast<uint32_t>(1000 * (wait))) {
        // sensor can interrupt us to let us know it is done early
        if (_SDI12Internal.available()) {
#ifdef MS_SDI12SENSORS_DEBUG_DEEP
          // if we're debugging print out early response
          MS_DEEP_DBG(F("    <<<"), _SDI12Internal.readStringUntil('\n'));
          _SDI12Internal.clearBuffer();
          break;
#else
          // if we're not debugging, just read the response to make
          // sure it's removed from the buffer
          _SDI12Internal.readStringUntil('\n');
          _SDI12Internal.clearBuffer();
          break;
#endif
        }
      }
      // Wait for anything else and clear it out
      delay(30);
      _SDI12Internal.clearBuffer();

      // get the results
      success = getResults(MS_SDI12_USE_CRC);
    } else {
      // If there's no measurement, need to make sure we send over all
      // of the "failed" result values
      MS_DBG(getSensorNameAndLocation(), F("is not currently measuring!"));
      for (uint8_t i = 0; i < _numReturnedValues; i++) {
        verifyAndAddMeasurementResult(i, static_cast<float>(MS_INVALID_VALUE));
      }
    }
  }

  // Empty the buffer and de-activate the SDI-12 Object
  deactivate();

  // Return success value when finished
  return finalizeMeasurementAttempt(success);
}