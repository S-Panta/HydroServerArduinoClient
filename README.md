# HydroServerArduinoClient

An Arduino MQTT and HTTP wrapper for sending sensor readings from a field logger to [HydroServer](https://github.com/hydroserver2).

Each sensor reading is an `Observation` (what was measured, which datastream it belongs to, and the value), and the library create Sensorthings specified [OGC SensorThings](https://www.ogc.org/standard/sensorthings/) JSON payload and sends it to the specified destination. There are two ways to send data:
- `HydroServerHTTPClient` : HTTP `POST` straight to the HydroServer SensorThings API 
- `HydroServerMQTTClient` : Publishes to an MQTT broker 

Both clients produce exactly the same payload, so you can switch between them without changing how you build observations.

It also includes small **modem wrappers** for the EnviroDIY Mayfly logger's Bee socket (Espressif ESP32 and Digi XBee S6B Wi-Fi) that handle power-up, Wi-Fi connection, NIST time sync, and the extra modem setup MQTT/HTTPS need.

---

## Supported hardware

The clients work with anything that implements Arduino's `Client` interface. The library has been tested with:

- **Arduino Uno R4 WiFi** (using the built-in `WiFiS3` / `WiFiClient`)
- **EnviroDIY Mayfly** with an **Espressif ESP32** Wi-Fi Bee (AT firmware)
- **EnviroDIY Mayfly** with a **Digi XBee S6B** Wi-Fi Bee

---

## Installation

1. **Get the library.** Clone this repository into your Arduino libraries folder:

   ```bash
   cd ~/Documents/Arduino/libraries      # adjust to your sketchbook location
   git clone git@github.com:S-Panta/HydroServerArduinoClient.git
   ```

2. **Install dependencies from the Arduino Library Manager:**
   - `ArduinoJson` (v7)
   - `ArduinoMqttClient`
   - `ArduinoHttpClient`

3. **Install EnviroDIY's TinyGSM fork manually** (only needed for the Mayfly modem wrappers).
   To run the example and use this library for Arduino Uno R4, you can skip this step.
   Do **not** install TinyGSM from the Library Manager. The modem classes used here rely on the fork maintained by EnviroDIY:

   ```bash
   cd ~/Documents/Arduino/libraries
   git clone https://github.com/EnviroDIY/TinyGSM
   ```


5. **Restart the Arduino IDE** and open one of the sketches under `examples/`.

> **Keep your secrets out of Git.** Every example reads Wi-Fi credentials (and API keys, where needed) from an `arduino_secrets.h` file next to the sketch. These files are already listed in `.gitignore`. Create your own like this:
>
> ```cpp
> #define WIFI_SSID "your-network"
> #define WIFI_PASS "your-password"
> #define PLAYGROUND_API_KEY "your-hydroserver-api-key" 
> ```

---

## Core concepts

### Observation

An `Observation` is one sensor reading:

```cpp
struct Observation {
  const char *observedProperty;  // what is being measured, e.g. "temperature"
  const char *datastreamId;      // HydroServer / SensorThings datastream UUID
  const char *sensorId;          // your identifier for the physical sensor
  double value;                  // the measured value

  Observation(const char *observedProperty, const char *datastreamId,
              const char *sensorId, double value = NAN);
};
```
### Payload

The payload for both HTTP and MQTT is in the json as shown below: 

```json
{
  "Datastream": { "@iot.id": "<datastreamId>" },
  "result": 21.7,
  "phenomenonTime": "2026-09-23T18:30:00Z"
}
```

### Timestamps

`phenomenonTime` must be an **ISO 8601 UTC** string (`YYYY-MM-DDTHH:MM:SSZ`). If See `getISO8601Time()` helper function used in the arduino uno examples and copy it into your sketch. Make sure the clock is set first. This step difference between Arduino Uno and Mayfly logger. Copy the code of  `syncRTCFromNTP()` function for Uno. For Mayfly, you can use the modem function to set the time `modem.getNISTTime()`

### MQTT topics

The MQTT client builds topics for you from the site code, client ID, and observation:

| Purpose | Topic |
| --- | --- |
| Observations | `<siteCode>/<clientId>/<sensorId>/<observedProperty>` |
| Last Will (offline notice) | `<siteCode>/<clientId>/lwt` |

For example, site `uwrl`, client `logger01`, sensor `temp-sensor-1`, property `temperature` publishes to `uwrl/logger01/temp-sensor-1/temperature`.

The site code should match your site in HydroServer (see [managing site metadata](https://hydroserver.org/user-guides/how-to/managing-site-metadata.html#managing-site-metadata)). Lowercase client IDs are recommended since they become part of the topic.

---

## Quick start: MQTT

A minimal Uno R4 WiFi sketch that publishes a reading every 30 seconds:

```cpp
#include <WiFiS3.h>
#include <HydroServerMQTTClient.h>
#include "arduino_secrets.h"

const char *MQTT_BROKER = "test.mosquitto.org";   // public test broker

WiFiClient wifiClient;
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);   // port defaults to 1883

Observation temperature = {"temperature", "<datastream-uuid>", "temp-sensor-1"};

void setup() {
  Serial.begin(115200);
  while (WiFi.begin(WIFI_SSID, WIFI_PASS) != WL_CONNECTED) delay(1000);

  // ... set the RTC here (see examples/UnoR4_Wifi/mqtt_basic) ...

  mqttClient.setSiteCode("mysite");        // required: first part of every topic
  mqttClient.setClientID("logger01");      // required for a stable topic / persistent session
  mqttClient.setLastWill("offline");       // optional; call before connecting

  if (!mqttClient.connectToBroker()) {
    Serial.print("MQTT connect failed, error: ");
    Serial.println(mqttClient.getConnectionError());
    while (true);
  }
}

void loop() {
  mqttClient.poll();                       // call often to keep the connection alive

  static unsigned long last = 0;
  if (millis() - last > 30000) {
    last = millis();
    temperature.value = analogRead(A0) * 0.1;   // replace with a real reading
    mqttClient.publishObservation(temperature, getISO8601Time());
  }
}
```

## Quick start: HTTP to HydroServer

`HydroServerHTTPClient` posts observations to `/api/sensorthings/v1.1/Observations` on your HydroServer instance, authenticating with an API key (`X-Api-Key` header).

```cpp
#include <HydroServerHTTPClient.h>
#include "arduino_secrets.h"

// client must be a TLS/secure client when using port 443
HydroServerHTTPClient hsClient(secureClient, "playground.hydroserver.org", 443, PLAYGROUND_API_KEY);

Observation temperature = {"temperature", "<datastream-uuid>", "temp-sensor-1"};

void loop() {
  temperature.value = readTemperature();
  bool ok = hsClient.publishObservation(temperature, getISO8601Time());
  Serial.println(ok ? "posted" : hsClient.getLastResponseBody());
  delay(60000);
}

## Using the Mayfly modem wrappers

On the Mayfly, the Wi-Fi Bee is driven by AT commands over `Serial1`. The modem classes wrap this so your sketch stays short:

```cpp
#include <HydroServerMQTTClient.h>
#include <modems/ExpressifESP32.h>      // or <modems/DigiXbeeS6B.h>

ExpressifESP32 modem(Serial1, 18);       // Bee serial port, Bee power pin (18 on Mayfly 1.x)
// ExpressifESP32 modem(Serial1, 18, Serial);   // same, but echoes AT traffic to Serial for debugging

HydroServerMQTTClient mqttClient(*modem.createClient(), "broker.example.org");

void setup() {
  Serial1.begin(57600);                  // ESP32 Bee baud rate
  modem.powerUp();
  modem.connectToInternet(WIFI_SSID, WIFI_PASS);
  rtc.setDateTime(modem.getNISTTime());  // sync the Mayfly DS3231 clock
  modem.extraSetupForMQTT();             // required before MQTT on these modems
  // ... then configure and connect mqttClient as in the MQTT quick start
}
```

## SDI-12 sensor drivers (Mayfly only)

`extra/sensors/` contains ModularSensors drivers for two SDI-12 sensors:

| Sensor | Files |
| --- | --- |
| FTS **DTS-12** turbidity sensor | `DTS12.h`, `DTS12.cpp` |
| Campbell Scientific **CS451** pressure transducer | `CS451.h`, `CS451.cpp` |

To use them on a Mayfly:

1. Install [EnviroDIY ModularSensors](https://github.com/EnviroDIY/ModularSensors) and its dependencies.
2. Copy the driver files from `extra/sensors/` into `ModularSensors/src/sensors/`.
3. Add the SDI-12 settings the DTS-12 needs to `ModSensorConfig.h`.
4. Include them in your sketch as usual:

   ```cpp
   #include <sensors/DTS12.h>
   #include <sensors/CS451.h>
   ```

Full instructions, the configuration lines, and the list of variables each sensor reports are in [`extra/sensors/README.md`](extra/sensors/README.md).

---

## Project layout

```
src/
  PublisherUtil.h            Observation struct and JSON payload builder
  HydroServerHTTPClient.*    HTTP(S) client for the SensorThings API
  HydroServerMQTTClient.*    MQTT client (wraps ArduinoMqttClient)
  modems/                    Mayfly Bee modem wrappers (ESP32, XBee S6B)
extra/
  sensors/                   Optional ModularSensors SDI-12 drivers (DTS-12, CS451); not compiled with the library
examples/                    Ready-to-run sketches, grouped by board
testing_example/             Development / debugging sketches
docs/                        API reference and architecture notes
```

Code style is enforced with `clang-format` via GitHub Actions on every push and pull request to `main`.

---

## Further documentation

- [`docs/apireference.md`](docs/apireference.md) — full reference for every `HydroServerMQTTClient` method.
- [`docs/architecture.md`](docs/architecture.md) — class structure of the library.
- [HydroServer documentation](https://hydroserver.org/) — setting up sites, datastreams, and API keys.
