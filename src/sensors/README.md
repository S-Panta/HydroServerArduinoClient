# DTS-12 Turbidity Sensor

Support for the **DTS-12 digital turbidity sensor** (SDI-12) in the [EnviroDIY ModularSensors](https://github.com/EnviroDIY/ModularSensors) library, tested with the Arduino IDE on an EnviroDIY Mayfly.

The DTS-12 returns 8 values per measurement:

| # | Variable class            | Value                         | Unit    | Default code    |
|---|---------------------------|-------------------------------|---------|-----------------|
| 0 | `DTS12_Mean_Turbidity`    | Mean turbidity                | NTU     | `DTS12Mean`     |
| 1 | `DTS12_Variance`          | Variance of turbidity         | NTU²    | `DTS12Variance` |
| 2 | `DTS12_Median_Turbidity`  | Median turbidity              | NTU     | `DTS12Median`   |
| 3 | `DTS12_BES_Turbidity`     | BES (trimean) turbidity       | NTU     | `DTS12BES`      |
| 4 | `DTS12_Min_Turbidity`     | Min of last 100 readings      | NTU     | `DTS12Min`      |
| 5 | `DTS12_Max_Turbidity`     | Max of last 100 readings      | NTU     | `DTS12Max`      |
| 6 | `DTS12_Temp`              | Water temperature             | °C      | `DTS12Temp`     |
| 7 | `DTS12_WipeStatus`        | Wipe status code              | –       | `DTS12Wipe`     |

Wipe status codes: `0` = wiped, `1` = too cold (wipe suppressed), `2` = no wipe for another reason, `3` = attempted wipe.


## 1. Installation

These files are **not** part of the official ModularSensors release. They must be copied into your installed copy of the library.

1. Find the library folder:
   - Windows: `Documents\Arduino\libraries\`
   - macOS/Linux: `~/Documents/Arduino/libraries/`
     (or `~/Arduino/libraries/`)
2. Clone [ModularSensors](https://github.com/EnviroDIY/ModularSensors) to the library folder.
3. Download the zip file for all the dependency for ModularSensors from [here](https://github.com/EnviroDIY/Libraries/blob/master/libraries.zip?raw=true"). For more information, go to official [installation guide](https://envirodiy.github.io/ModularSensors/page_getting_started.html) for modularsensors.
4. Copy DTS12.h and DTS12.cpp files into the `src/sensors/` folder inside ModularSensors: The Arduino IDE compiles every `.cpp` under a library's `src/` folder automatically.


## 2. Required library configuration

The DTS-12 reports SDI-12 version 1.2 and **does not respond to the concurrent measurement command** that ModularSensors sends by default (`aCC!` a concurrent measurement with CRC where a is the address of DTS-12 sensor). See Section 3.3.1 of DTS-12 Sensor [User Manual](https://s3.amazonaws.com/Product_Sensors/700-DTS-12.pdf). 

Open `ModularSensors/src/ModSensorConfig.h` and add these lines **inside the header guard** (after the `#define SRC_MODSENSORCONFIG_H_` line and before the final `#endif`):

```cpp
// ---- Required for FTS DTS-12 ----
#define MS_SDI12_NON_CONCURRENT   // send aM! instead of aC!
#define MS_SDI12_NO_CRC_CHECK     // send 1M!  instead of 1MC!
```


## 3. Sensor setup

- **SDI-12 address:** the DTS-12 ships at address `0`. Change it to a unique
  address (e.g. `1`) before use, using the Arduino-SDI12 library's run [b_address_change](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/b_address_change).ino. You can also use CR350 or any datalogger and run aAb! command where a is the old address and b is the new SDI-12 address.

- **Timing:** each reading takes about 15 s warm-up (includes the wipe cycle on power-up) plus up to ~36 s of measurement. Use a logging interval of **at least 2 minutes**. 

## 5. Debugging (Arduino IDE)

ModularSensors has built-in debug printing that goes to the Serial Monitor. All of the DTS-12's SDI-12 communication is handled by the parent class `SDI12Sensors`, so its debug output shows everything the DTS-12 is doing

Open `ModularSensors/src/ModSensorDebugConfig.h`. It already contains the debug options, commented out. Uncomment these two lines.

```cpp
#define MS_SDI12SENSORS_DEBUG       
#define MS_SDI12SENSORS_DEBUG_DEEP 
```