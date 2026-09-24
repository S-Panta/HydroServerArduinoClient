# HydroServerArduinoClient

An Arduino MQTT and HTTP wrapper for sending sensor readings from a field logger to [HydroServer](https://github.com/hydroserver2).

You describe each reading as an `Observation` (what was measured, which datastream it belongs to, and the value), and the library create Sensorthings specified [OGC SensorThings](https://www.ogc.org/standard/sensorthings/) JSON payload and sends it for you. There are two ways to send data:

| Client | How it sends | When to use it |
| --- | --- | --- |
| `HydroServerHTTPClient` | HTTP `POST` straight to the HydroServer SensorThings API | No extra server; just http post |
| `HydroServerMQTTClient` | Publishes to an MQTT broker | You already run a broker, want lightweight/low-power messaging, or need two-way messaging. A separate service (e.g. a Python script) subscribes to the broker and forwards data into HydroServer. |

Both clients produce exactly the same payload, so you can switch between them without changing how you build observations.

It also includes small **modem wrappers** for the EnviroDIY Mayfly logger's Bee socket (Espressif ESP32 and Digi XBee S6B Wi-Fi) that handle power-up, Wi-Fi connection, NIST time sync, and the extra modem setup MQTT/HTTPS need.

---

## Contents

- [Supported hardware](#supported-hardware)
- [Installation](#installation)
- [Core concepts](#core-concepts)
- [Quick start: MQTT](#quick-start-mqtt)
- [Quick start: HTTP(S) to HydroServer](#quick-start-https-to-hydroserver)
- [Using the Mayfly modem wrappers](#using-the-mayfly-modem-wrappers)
- [Subscribing to MQTT messages](#subscribing-to-mqtt-messages)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)
- [Project layout](#project-layout)
- [Further documentation](#further-documentation)

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
   git clone <this-repo-url> HydroServerArduinoClient
   ```

2. **Install dependencies from the Arduino Library Manager:**
   - `ArduinoJson` (v7)
   - `ArduinoMqttClient`
   - `ArduinoHttpClient`

3. **Install EnviroDIY's TinyGSM fork manually** (only needed for the Mayfly modem wrappers).
   Do **not** install TinyGSM from the Library Manager — the modem classes rely on the fork maintained by EnviroDIY:

   ```bash
   cd ~/Documents/Arduino/libraries
   git clone https://github.com/EnviroDIY/TinyGSM
   ```

   The modem wrappers also use `StreamDebugger`, and the Mayfly examples use `Sodaq_DS3231` for the real-time clock. If you already have **EnviroDIY ModularSensors** installed, you have all of these.

4. **Restart the Arduino IDE** and open one of the sketches under `examples/`.

> **Keep your secrets out of Git.** Every example reads Wi-Fi credentials (and API keys, where needed) from an `arduino_secrets.h` file next to the sketch. These files are already listed in `.gitignore`. Create your own like this:
>
> ```cpp
> #define WIFI_SSID "your-network"
> #define WIFI_PASS "your-password"
> #define PLAYGROUND_API_KEY "your-hydroserver-api-key"   // HTTP examples only
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
  double value = 0.0;            // the measured value
};
```

You can create one in a single line (fields in the order above) and set the value each time you take a reading:

```cpp
Observation temperature = {"temperature", "<datastream-uuid>", "temp-sensor-1"};
temperature.value = 21.7;
```

### Payload

Both clients send the same SensorThings-style JSON:

```json
{
  "Datastream": { "@iot.id": "<datastreamId>" },
  "result": 21.7,
  "phenomenonTime": "2026-09-23T18:30:00Z"
}
```

### Timestamps

`phenomenonTime` must be an **ISO 8601 UTC** string (`YYYY-MM-DDTHH:MM:SSZ`). The library have the time helper. Every example includes a small `getISO8601Time()` helper that formats the board's real-time clock. Copy it into your sketch. Make sure the clock is set first (NTP on the Uno R4, `modem.getNISTTime()` on the Mayfly).

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

**Tips**

- Call `setSiteCode()` and `setClientID()` **before** `connectToBroker()`, `setLastWill()`, or `publishObservation()`. If the site code is missing, the topic is malformed and the broker will drop the connection.
- Call `poll()` regularly in `loop()`. If the broker hears nothing for about 1.5× the keep-alive interval (default 60 s, change with `setKeepAliveInterval()`), it disconnects you.
- To send several observations with one timestamp, put pointers to them in an array and use `publishAll()`:

  ```cpp
  Observation *all[] = {&temperature, &ph};
  mqttClient.publishAll(all, 2, getISO8601Time());
  ```

- **Testing with a public broker:** `test.mosquitto.org` is free and open. Subscribe to your exact topic (e.g. `mosquitto_sub -h test.mosquitto.org -t "mysite/#"`) rather than `#`, which would show you every message on the broker.

### Other configuration

| Method | What it does |
| --- | --- |
| `setAuthentication(user, pass)` | Username/password for brokers that need login. |
| `setCleanSession(bool)` | `false` (default) asks the broker to remember your subscriptions and queued messages between reconnects. Only matters for QoS 1/2. |
| `setKeepAliveInterval(seconds)` | How often a keep-alive ping is sent. Default 60 s. |
| `setConnectionTimeout(seconds)` | How long to wait for the network connection to the broker. Default 20 s. |
| `connectToBroker(broker, port, clientId, user, pass)` | Set connection details and connect in one call (useful with the `HydroServerMQTTClient(client)` constructor). |
| `isConnected()` / `getConnectionError()` | Check the connection and read the last error code. |

---

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
```

Useful methods:

| Method | What it does |
| --- | --- |
| `publishObservation(obs, time)` | POSTs one observation. Returns `true` if the server replied `200`. |
| `publishAll(obsArray, count, time)` | POSTs several observations with the same timestamp. |
| `isDatastreamAvailable(id)` | Checks that a datastream UUID exists on the server — handy at startup to catch typos. |
| `getLastResponseBody()` | The server's response to the last request (read this when something fails). |
| `setApiKey(key)` | Change the API key after construction. |

The server defaults to `playground.hydroserver.org` on port 443. Create a datastream and API key in your HydroServer account, then copy the datastream UUID into your `Observation`.

---

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

| Method | Purpose |
| --- | --- |
| `powerUp()` | Switches on the Bee socket power pin. |
| `connectToInternet(ssid, pass, timeoutMs)` | Joins Wi-Fi (default timeout 60 s). |
| `isInternetAvailable()` | Checks the connection. |
| `getNISTTime()` | Returns the current Unix time from a NIST time server — use it to set the RTC. |
| `createClient()` / `createSecureClient()` | Returns a plain or TLS `Client*` to pass into the HTTP or MQTT client. |
| `extraSetupForMQTT()` | Modem-specific setup MQTT needs. **Call it before connecting to the broker.** |
| `extraSetupForHTTPS(host, port)` | *(ESP32 only)* Sets up SNI and the SSL connection for HTTPS posts. |

---

## Subscribing to MQTT messages

The MQTT client can also listen for messages, for example to receive commands or values from another device.

```cpp
void messageReceived(int size) {
  Serial.print("Topic: ");
  Serial.println(mqttClient.messageTopic());
  Serial.println(mqttClient.readMessage());
}

void setup() {
  // ... connect first ...
  mqttClient.onMessage(messageReceived);
  mqttClient.subscribe("python_publisher/sensorid/temperature_celsius");
}

void loop() {
  mqttClient.poll();   // incoming messages are only delivered while polling
}
```

- `subscribe(topic)` automatically puts your site code in front, so the call above subscribes to `<siteCode>/python_publisher/sensorid/temperature_celsius`. Subscriptions use QoS 0.
- `unsubscribe(topic)` does **not** add the site code — pass the full topic.
- If the incoming message is an observation payload, call `deserializePayload()` inside your callback and then `getLatestValueofTopic(topic)` to get the `result` as a number (see the `mqtt_subscribe` examples).

---

## Examples

Open these from **File → Examples → HydroServerArduinoClient** in the Arduino IDE.

| Board | Example | What it shows |
| --- | --- | --- |
| Uno R4 WiFi | `mqtt_basic` | Connect to Wi-Fi, sync time via NTP, set a Last Will, publish two observations on a timer. **Best place to start.** |
| Uno R4 WiFi | `mqtt_subscribe` | Receive and parse messages from subscribed topics. |
| Uno R4 WiFi | `http_post_hydroserver` | Post observations directly to HydroServer. |
| Mayfly + ESP32 | `mqtt_basic` | Publish on clock-aligned intervals (e.g. every minute on the minute) using the DS3231 RTC. |
| Mayfly + ESP32 | `mqtt_subscribe` | Subscribe and read incoming values. |
| Mayfly + ESP32 | `http_post_hydroserver` / `https_post_hydroserver` | Post to HydroServer over HTTP or HTTPS. |
| Mayfly + ESP32 | `turbidity_sdi12_mqtt_sd` | Combine with EnviroDIY ModularSensors: read sensors, log to SD card, and publish over MQTT. |
| Mayfly + XBee S6B | `mqtt_basic` | MQTT publishing with the Digi XBee Wi-Fi Bee. |
| Mayfly + XBee S6B | `rtc_mayfly` | Set the Mayfly clock from a NIST time server. |

The `testing_example/` folder contains development and debugging sketches (raw AT command tests, ThingSpeak MQTT, etc.). They are not part of the public API.

---

## Troubleshooting

| Symptom | Things to check |
| --- | --- |
| `connectToBroker()` returns 0 | Is a broker address set? Print `getConnectionError()`. Check the port (1883 is unencrypted MQTT) and credentials. |
| Connected, but broker drops the connection right after publishing | Site code or client ID not set, producing a malformed topic. Call `setSiteCode()` and `setClientID()` first. |
| Connection drops after a minute or two | `poll()` isn't being called often enough in `loop()`. |
| MQTT on the Mayfly never connects | Did you call `modem.extraSetupForMQTT()`? Is `Serial1` at 57600 baud for the ESP32? |
| Nothing arrives at HydroServer (MQTT) | MQTT does not write to HydroServer by itself — make sure your forwarding service is subscribed to the right topics. |
| HTTP post fails | Print `getLastResponseBody()`. Check the API key, that the datastream UUID exists (`isDatastreamAvailable()`), and that the timestamp is valid ISO 8601 UTC. |
| Payload or timestamps look wrong | The RTC probably wasn't set. Sync time before publishing. |
| Large payloads get cut off | ArduinoMqttClient's default buffer is small (~256 bytes); keep IDs and property names short. |

Need to see what the modem is doing? Construct it with a debug stream (e.g. `ExpressifESP32 modem(Serial1, 18, Serial);`) to echo every AT command to the Serial Monitor.

---

## Project layout

```
src/
  PublisherUtil.h            Observation struct and JSON payload builder
  HydroServerHTTPClient.*    HTTP(S) client for the SensorThings API
  HydroServerMQTTClient.*    MQTT client (wraps ArduinoMqttClient)
  modems/                    Mayfly Bee modem wrappers (ESP32, XBee S6B)
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
