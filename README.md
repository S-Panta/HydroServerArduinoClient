# HydroServerArduinoClient

An Arduino library for sending sensor readings from a field logger to [HydroServer](https://github.com/hydroserver2), either directly over HTTP(S) or through an MQTT broker.

You describe each reading as an `Observation`: what was measured, which HydroServer datastream it belongs to, and the value. The library turns it into an [OGC SensorThings](https://www.ogc.org/standard/sensorthings/) JSON payload and sends it with one of two clients:

| Client | Sends to | Use when |
| --- | --- | --- |
| `HydroServerHTTPClient` | HydroServer's SensorThings API (`POST /api/sensorthings/v1.1/Observations`) | The logger can reach HydroServer directly and you have an API key. |
| `HydroServerMQTTClient` | Any MQTT broker | You already run a broker, or many loggers share one connection point. |

Both clients send the same payload, so you can switch between them without changing how you build observations.

> **MQTT does not reach HydroServer on its own.** The broker only relays messages. To get MQTT data into HydroServer you need a separate service that subscribes to the broker and writes the observations to HydroServer.

The library also includes **modem wrappers** for the EnviroDIY Mayfly's Bee socket (Espressif ESP32 and Digi XBee S6B Wi-Fi Bees). They handle power-up, Wi-Fi connection, clock sync and the extra modem setup that MQTT and HTTPS need.

---

## Contents

- [Supported hardware](#supported-hardware)
- [Installation](#installation)
- [Core concepts](#core-concepts)
- [Quick start: MQTT](#quick-start-mqtt)
- [Quick start: HTTP to HydroServer](#quick-start-http-to-hydroserver)
- [Using the Mayfly modem wrappers](#using-the-mayfly-modem-wrappers)
- [Examples](#examples)
- [SDI-12 sensor drivers](#sdi-12-sensor-drivers-mayfly-only)
- [Troubleshooting](#troubleshooting)
- [Project layout](#project-layout)
- [Further documentation](#further-documentation)
- [License](#license)

---

## Supported hardware

The clients work with any network client that implements Arduino's `Client` interface. The library has been tested with:

| Board | Network | Client to pass in |
| --- | --- | --- |
| Arduino Uno R4 WiFi | Built-in Wi-Fi (`WiFiS3`) | `WiFiClient`, or `WiFiSSLClient` for HTTPS |
| EnviroDIY Mayfly v1.x | Espressif ESP32 Wi-Fi Bee (AT firmware) | `*modem.createClient()`, or `*modem.createSecureClient()` for HTTPS |
| EnviroDIY Mayfly v1.x | Digi XBee S6B Wi-Fi Bee | `*modem.createClient()` (no TLS) |

---

## Installation

1. **Get the library.** Clone this repository into your Arduino `libraries` folder:

   ```bash
   cd ~/Documents/Arduino/libraries      # adjust to your sketchbook location
   git clone https://github.com/S-Panta/HydroServerArduinoClient.git
   ```

2. **Install these from the Arduino Library Manager:**
   - `ArduinoJson` (v7 or later)
   - `ArduinoMqttClient`
   - `ArduinoHttpClient`
   - `StreamDebugger`

3. **Install EnviroDIY's TinyGSM fork manually.** Do **not** install TinyGSM from the Library Manager; the modem wrappers need the fork maintained by EnviroDIY. The wrappers are compiled with the library on every board, so install it even if you only use an Uno R4.

   ```bash
   cd ~/Documents/Arduino/libraries
   git clone https://github.com/EnviroDIY/TinyGSM.git
   ```

4. **Install the libraries your board's examples use:**
   - Uno R4 WiFi: `NTPClient` (Library Manager)
   - Mayfly: `Sodaq_DS3231`, and [EnviroDIY ModularSensors](https://github.com/EnviroDIY/ModularSensors) for the logger examples. The [EnviroDIY libraries bundle](https://github.com/EnviroDIY/Libraries) contains both.

5. **Restart the Arduino IDE.** The examples appear under **File → Examples → HydroServerArduinoClient**.

### Keep your secrets out of Git

Every example reads Wi-Fi credentials (and an API key, where needed) from an `arduino_secrets.h` file in the sketch folder. These files are listed in `.gitignore`, so you have to create your own before compiling:

```cpp
// arduino_secrets.h
#define WIFI_SSID "your-network"
#define WIFI_PASS "your-password"          // use 0 for an open network
#define PLAYGROUND_API_KEY "your-api-key"  // HTTPS examples only
#define LOCAL_API_KEY "your-api-key"       // Mayfly plain-HTTP example only
```

---

## Core concepts

### Observation

An `Observation` is one sensor reading:

```cpp
struct Observation {
  const char *observedProperty;  // what is measured, e.g. "temperature"
  const char *datastreamId;      // HydroServer datastream UUID
  const char *sensorId;          // your identifier for the physical sensor
  double value;                  // the measured value (NAN until you set it)
};

Observation temperature = {"temperature", "<datastream-uuid>", "temp-sensor-1"};
temperature.value = 21.7;
```

The struct stores pointers, not copies, so use string literals or global/static strings.

### Payload

Both clients send this JSON:

```json
{
  "Datastream": { "@iot.id": "<datastreamId>" },
  "result": 21.7,
  "phenomenonTime": "2026-09-23T18:30:00Z"
}
```

### Timestamps

The library never reads a clock. You pass the timestamp to every publish call as an **ISO 8601 UTC** string (`YYYY-MM-DDTHH:MM:SSZ`), so the board's clock must be set first:

- **Uno R4 WiFi:** set the on-board RTC from an NTP server. Copy `syncRTCFromNTP()` and `getISO8601Time()` from any of the `UnoR4_Wifi` examples.
- **Mayfly:** set the DS3231 clock with `rtc.setDateTime(modem.getNISTTime())`, then format it with the `getISO8601Time()` helper in the `mayfly_esp32` examples. ModularSensors loggers can use `Logger::formatDateTime_ISO8601(Logger::markedLocalUnixTime)` instead.

### MQTT topics

The MQTT client builds topics from the site code, client ID and observation:

| Purpose | Topic |
| --- | --- |
| Observations | `<siteCode>/<clientId>/<sensorId>/<observedProperty>` |
| Last Will (offline notice) | `<siteCode>/<clientId>/lwt` |

For example, site `uwrl`, client `logger01`, sensor `temp-sensor-1` and property `temperature` publish to `uwrl/logger01/temp-sensor-1/temperature`.

**Always call both `setSiteCode()` and `setClientID()` before connecting.** Use your HydroServer site's code (see [managing site metadata](https://hydroserver.org/user-guides/how-to/managing-site-metadata.html#managing-site-metadata)). Topics are case-sensitive, so lowercase IDs and property names are recommended.

---

## Quick start: MQTT

A minimal Uno R4 WiFi sketch that publishes a reading every 30 seconds. See [`examples/UnoR4_Wifi/mqtt_basic`](examples/UnoR4_Wifi/mqtt_basic) for the complete version, including the clock helpers.

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
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  syncRTCFromNTP();                        // see the example

  mqttClient.setSiteCode("mysite");        // required: first part of every topic
  mqttClient.setClientID("logger01");      // required: second part of every topic
  mqttClient.setLastWill("offline");       // optional; call after the two setters above

  if (!mqttClient.connectToBroker()) {
    Serial.print("MQTT connect failed, error: ");
    Serial.println(mqttClient.getConnectionError());
    while (true);
  }
}

void loop() {
  mqttClient.poll();                       // call on every pass to keep the connection alive

  static unsigned long last = 0;
  if (millis() - last > 30000) {
    last = millis();
    temperature.value = analogRead(A0) * 0.1;   // replace with a real reading
    mqttClient.publishObservation(temperature, getISO8601Time());
  }
}
```

---

## Quick start: HTTP to HydroServer

`HydroServerHTTPClient` posts each observation to `/api/sensorthings/v1.1/Observations` on your HydroServer instance and authenticates with an API key (`X-Api-Key` header). See [`examples/UnoR4_Wifi/http_post_hydroserver`](examples/UnoR4_Wifi/http_post_hydroserver) for the complete version.

```cpp
#include <WiFiS3.h>
#include <HydroServerHTTPClient.h>
#include "arduino_secrets.h"

WiFiSSLClient sslClient;   // port 443 needs a TLS client
HydroServerHTTPClient hsClient(sslClient, "playground.hydroserver.org", 443,
                               PLAYGROUND_API_KEY);

Observation temperature = {"temperature", "<datastream-uuid>", "temp-sensor-1"};

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  syncRTCFromNTP();                        // see the example

  // Optional startup check: catches a mistyped UUID or a key without access
  if (!hsClient.isDatastreamAvailable(temperature.datastreamId)) {
    Serial.println(hsClient.getLastResponseBody());
  }
}

void loop() {
  temperature.value = analogRead(A0) * 0.1;   // replace with a real reading
  if (!hsClient.publishObservation(temperature, getISO8601Time())) {
    Serial.println(hsClient.getLastResponseBody());
  }
  delay(60000);
}
```

Always pass the API key, either in the constructor or with `setApiKey()`, before the first request.

---

## Using the Mayfly modem wrappers

On the Mayfly, the Wi-Fi Bee is driven by AT commands over `Serial1`. The modem classes wrap this so your sketch stays short:

```cpp
#include <HydroServerMQTTClient.h>
#include <Sodaq_DS3231.h>
#include <modems/ESP32Bee.h>   // or <modems/DigiXbeeS6B.h>; include only one
#include "arduino_secrets.h"

// Bee serial port and Bee power pin (18 on Mayfly v1.x)
ESP32Bee modem(Serial1, 18);
// ESP32Bee modem(Serial1, 18, Serial);   // also echoes AT traffic to Serial

// createClient() allocates a new client, so call it once at global scope
HydroServerMQTTClient mqttClient(*modem.createClient(), "broker.example.org");

void setup() {
  Serial1.begin(57600);                   // ESP32 Bee: 57600 baud; XBee S6B: 9600
  modem.powerUp();
  delay(1000);
  modem.connectToInternet(WIFI_SSID, WIFI_PASS);

  rtc.begin();
  rtc.setDateTime(modem.getNISTTime());   // sets the Mayfly's DS3231 clock (UTC)

  modem.extraSetupForMQTT();              // required on the ESP32 before MQTT
  // ...then configure and connect mqttClient as in the MQTT quick start
}
```

For HTTPS on the ESP32, use `*modem.createSecureClient()` and call `modem.extraSetupForHTTPS(host, 443)` instead of `extraSetupForMQTT()`. The XBee S6B does not support TLS here, so use plain MQTT or HTTP with it.

---

## Examples

Open them from **File → Examples → HydroServerArduinoClient**. Each sketch starts with a comment block listing the hardware, required libraries and what to configure.

| Example | Board | Shows |
| --- | --- | --- |
| `UnoR4_Wifi/mqtt_basic` | Uno R4 WiFi | Publish observations over MQTT |
| `UnoR4_Wifi/mqtt_subscribe` | Uno R4 WiFi | Publish and receive values on subscribed topics |
| `UnoR4_Wifi/http_post_hydroserver` | Uno R4 WiFi | Post to HydroServer over HTTPS |
| `mayfly_esp32/mqtt_basic` | Mayfly + ESP32 | Publish on clock-aligned intervals over MQTT |
| `mayfly_esp32/mqtt_subscribe` | Mayfly + ESP32 | Publish and receive values on subscribed topics |
| `mayfly_esp32/https_post_hydroserver` | Mayfly + ESP32 | Post to HydroServer over HTTPS |
| `mayfly_esp32/http_post_hydroserver` | Mayfly + ESP32 | Post to a self-hosted HydroServer over plain HTTP |
| `mayfly_esp32/mqtt_publish_with_modular_sensor` | Mayfly + ESP32 | ModularSensors logger (battery, board temperature) with sleep between readings |
| `mayfly_xbee/mqtt_basic` | Mayfly + XBee S6B | Publish the on-board SHT4x temperature over MQTT |
| `dts12_with_mqtt_publish` | Mayfly + ESP32 | FTS DTS-12 turbidity sensor via ModularSensors |
| `cs451_with_mqtt_publish` | Mayfly + ESP32 | Campbell Scientific CS451 pressure transducer via ModularSensors |

The DTS-12 and CS451 examples also need the drivers from [`extra/sensors`](extra/sensors/README.md).

---

## SDI-12 sensor drivers (Mayfly only)

`extra/sensors/` contains [ModularSensors](https://github.com/EnviroDIY/ModularSensors) drivers for two SDI-12 sensors that ModularSensors doesn't support yet:

| Sensor | Files |
| --- | --- |
| FTS **DTS-12** turbidity sensor | `DTS12.h`, `DTS12.cpp` |
| Campbell Scientific **CS451** pressure transducer | `CS451.h`, `CS451.cpp` |

They are not compiled with this library. To use them:

1. Install ModularSensors and its dependencies.
2. Copy the driver files from `extra/sensors/` into `ModularSensors/src/sensors/`.
3. Add the SDI-12 settings the drivers need to `ModSensorConfig.h`.
4. Include them in your sketch:

   ```cpp
   #include <sensors/DTS12.h>
   #include <sensors/CS451.h>
   ```

Full instructions, the configuration lines and the variables each sensor reports are in [`extra/sensors/README.md`](extra/sensors/README.md).

---

## Troubleshooting

| Symptom | Likely cause |
| --- | --- |
| `connectToBroker()` returns 0 | Check `getConnectionError()`; the codes are listed in the [API reference](docs/apireference.md#int-getconnectionerror). `-2` usually means the broker host or port can't be reached. |
| Messages never arrive / connection drops | `poll()` isn't called often enough, or the keep-alive interval is shorter than the time between `poll()` calls (for example while the logger sleeps). |
| Data lands on an unexpected topic | `setSiteCode()` or `setClientID()` wasn't called, or IDs differ in case. Print `getObservationTopic(observation)` to check. |
| ESP32 Bee connects but MQTT/HTTP stalls | `modem.extraSetupForMQTT()` (or `extraSetupForHTTPS()`) wasn't called after `connectToInternet()`. |
| HTTPS to the HydroServer playground fails on the ESP32 | `extraSetupForHTTPS(host, 443)` is required so the server receives the host name (SNI). |
| HydroServer rejects the post | Print `getLastResponseBody()`. Common causes: wrong datastream UUID, an API key without write access, or a clock that was never set (timestamps in 2000). |

---

## Project layout

```
src/
  PublisherUtil.h            Observation struct and JSON payload builder
  HydroServerHTTPClient.*    HTTP(S) client for the SensorThings API
  HydroServerMQTTClient.*    MQTT client (wraps ArduinoMqttClient)
  modems/                    Mayfly Bee modem wrappers (ESP32, XBee S6B)
examples/                    Example sketches, see the table above
extra/
  sensors/                   ModularSensors SDI-12 drivers (DTS-12, CS451); not compiled with the library
testing_example/             Development and debugging sketches (not maintained as examples)
docs/                        API reference and architecture notes
```

Code style is checked with `clang-format` by a GitHub Action on every push and pull request to `main`.

---

## Further documentation

- [`docs/apireference.md`](docs/apireference.md): every public class and method.
- [`docs/architecture.md`](docs/architecture.md): how the library is structured.
- [HydroServer documentation](https://hydroserver.org/): setting up sites, datastreams and API keys.

---

## License

Released under the BSD 3-Clause License. See [`LICENSE`](LICENSE).
