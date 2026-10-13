# DTS - 12 and CS451 Support for ModularSensors
ModularSensors does not include drivers for the FTS DTS-12 turbidity sensor or the Campbell Scientific CS451 pressure transducer. This folder provides them. Both sensors communicate over SDI-12 and were tested on an EnviroDIY Mayfly using the Arduino IDE.
| Sensor | Files | Measures |
|---|---|---|
| FTS **DTS-12** turbidity sensor | `DTS12.h`, `DTS12.cpp` | Turbidity statistics + water temperature |
| Campbell Scientific **CS451** pressure transducer | `CS451.h`, `CS451.cpp` | Gage height (water level) + water temperature |

Both classes inherit from `SDI12Sensors`, so all SDI-12 communication (acknowledge, measure, data requests) goes through the ModularSensors parent class.

---

## 1. Installation

1. Find your Arduino library folder:
   - Windows: `Documents\Arduino\libraries\`
   - macOS/Linux: `~/Documents/Arduino/libraries/` (or `~/Arduino/libraries/`)
2. Clone [ModularSensors](https://github.com/EnviroDIY/ModularSensors) into the library folder.
3. Download the zip of all ModularSensors dependencies from [here](https://github.com/EnviroDIY/Libraries/blob/master/libraries.zip?raw=true). See the official [getting started guide](https://envirodiy.github.io/ModularSensors/page_getting_started.html) for more detail.
4. Copy `DTS12.h`, `DTS12.cpp`, `CS451.h` and `CS451.cpp` into `ModularSensors/src/sensors/`. The Arduino IDE compiles every `.cpp` under a library's `src/` folder automatically.
5. Include them in your sketch:
   ```cpp
#include <sensors/CS451.h>
#include <sensors/DTS12.h>
   ```

---

## 2. Required library configuration

The DTS-12 reports SDI-12 version 1.2 and **does not respond to the concurrent measurement command** that ModularSensors sends by default (`aCC!`, a concurrent measurement with CRC, where `a` is the sensor address). See Section 3.3.1 of the DTS-12 [User Manual](https://s3.amazonaws.com/Product_Sensors/700-DTS-12.pdf).

Open `ModularSensors/src/ModSensorConfig.h` and add these lines **inside the header guard** (after `#define SRC_MODSENSORCONFIG_H_` and before the final `#endif`):

```cpp
// ---- Required for FTS DTS-12 (also used by the CS451 driver) ----
#define MS_SDI12_NON_CONCURRENT // send aM! instead of aC!
#define MS_SDI12_NO_CRC_CHECK   // send aM! instead of aMC!
```

---

## 3. DTS-12 Turbidity Sensor

| # | Variable class            | Value                         | Unit    | Default code    |
|---|---------------------------|-------------------------------|---------|-----------------|
| 0 | `DTS12_Mean_Turbidity`    | Mean turbidity                | NTU     | `DTS12Mean`     |
| 1 | `DTS12_Variance`          | Variance of turbidity         | NTU²    | `DTS12Variance` |
| 2 | `DTS12_Median_Turbidity`  | Median turbidity              | NTU     | `DTS12Median`   |
| 3 | `DTS12_BES_Turbidity`     | BES (trimean) turbidity       | NTU     | `DTS12BES`      |
| 4 | `DTS12_Min_Turbidity`     | Min of last 100 readings      | NTU     | `DTS12Min`      |
| 5 | `DTS12_Max_Turbidity`     | Max of last 100 readings      | NTU     | `DTS12Max`      |
| 6 | `DTS12_Temp`              | Water temperature             | °C      | `DTS12Temp`     |

### Setup

- **SDI-12 address:** the DTS-12 ships at address `0`. Change it to a unique address (e.g. `1`) before use, with the Arduino-SDI12 example [b_address_change](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/b_address_change). You can also use a CR350 or another datalogger and send `aAb!`, where `a` is the old address and `b` is the new one.
- **Timing (in `DTS12.h`):** warm-up 9 s (includes the wiper cycle on power-up) and measurement time 40 s. Use a logging interval of **at least 2 minutes**.

### Example

```cpp
#include <sensors/DTS12.h>
DTS12 dts12('1', sensorPowerPin, 7, 1);  // address, power pin, data pin, readings to average

new DTS12_Median_Turbidity(&dts12, "UUID");
new DTS12_Temp(&dts12, "UUID");
```

---

## 4. CS451 Pressure Transducer
Official manual: https://s.campbellsci.com/documents/us/manuals/cs451-cs456.pdf

| # | Variable class      | Value                     | Unit | Default code      |
|---|---------------------|---------------------------|------|-------------------|
| 0 | `CS451_GageHeight`  | Gage height (water level) | mm   | `CS451GageHeight` |
| 1 | `CS451_Temp`        | Water temperature         | °C   | `CS451Temp`       |

### How the driver works

- The CS451's default pressure output is psig (1 psi = 2.31 ft of water). This sensor has been configured to report **level in mm and temperature in °C**. If you use a different sensor, set its units first or change `CS451_PRESSURE_UNIT_NAME` to match.
- The sensor is configured to average 50 measurements (`aXCONFIG2=50!`). The averaged result is read with the **`aM8!`** command instead of the standard `aM!`.
- A single measurement cycle takes under 1.5 s, but averaging 50 readings takes about 52 seconds. Use a logging interval of **at least 2 minutes**.

### Setup

- **SDI-12 address:** the CS451 ships at address `0`. Change it to a unique address (e.g. `2`) the same way as the DTS-12.
- **Power:** the CS451 needs 6–18 V. Pass the pin that actually switches its power (on a Mayfly, `sensorPowerPin = 22`). Use `-1` only if it is powered continuously.

### Example

```cpp
#include <sensors/CS451.h>
const char  CS451Address = '2';
const int8_t CS451DataPin = 7;
CS451 cs451(CS451Address, sensorPowerPin, CS451DataPin, 1);

new CS451_GageHeight(&cs451, "UUID");
new CS451_Temp(&cs451, "UUID");
```

---

## 5. Debugging (Arduino IDE)

ModularSensors has built-in debug printing to the Serial Monitor. Because both sensors use `SDI12Sensors` for communication, its debug output shows every command and reply.

Open `ModularSensors/src/ModSensorDebugConfig.h`. Uncomment the first two lines and add the last two:

```cpp
#define MS_SDI12SENSORS_DEBUG
#define MS_SDI12SENSORS_DEBUG_DEEP
#define MS_DTS12_DEBUG
#define MS_CS451_DEBUG
```
