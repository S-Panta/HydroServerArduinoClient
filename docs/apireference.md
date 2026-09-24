# API Reference

Complete reference for the public API of **HydroServerArduinoClient**.

- [Observation and payload](#observation-and-payload) — `PublisherUtil.h`
- [HydroServerMQTTClient](#hydroservermqttclient) — `HydroServerMQTTClient.h`
- [HydroServerHTTPClient](#hydroserverhttpclient) — `HydroServerHTTPClient.h`
- [Modem wrappers](#modem-wrappers) — `modems/modem.h`, `modems/ExpressifESP32.h`, `modems/DigiXbeeS6B.h`

For installation and a guided walkthrough, see the [README](../README.md). For how the pieces fit together, see [architecture.md](architecture.md).

---

## Observation and payload

```cpp
#include <PublisherUtil.h>   // included automatically by both clients
```

### `struct Observation`

One sensor reading.

```cpp
struct Observation {
  const char *observedProperty;
  const char *datastreamId;
  const char *sensorId;
  double value = 0.0;
};
```

| Field | Description | Used in |
| --- | --- | --- |
| `observedProperty` | Name of what is measured, e.g. `"temperature"`. | MQTT topic |
| `datastreamId` | UUID of the HydroServer / SensorThings datastream the value belongs to. | Payload |
| `sensorId` | Your identifier for the physical sensor. | MQTT topic |
| `value` | The measured value. Defaults to `0.0`. | Payload (`result`) |

The struct stores **pointers**, not copies. Strings you assign must stay valid for as long as the observation is used — string literals and global/static buffers are safe; local `char` arrays inside a function are not.

```cpp
Observation temperature = {"temperature", "019f246b-...", "temp-sensor-1"};
temperature.value = 21.7;
```

### `ObservationPayload::serialize()`

```cpp
String ObservationPayload::serialize(const Observation &observation, const char *phenomenonTime);
```

Builds the JSON body both clients send. You normally don't call this yourself.

```json
{
  "Datastream": { "@iot.id": "<datastreamId>" },
  "result": "value",
  "phenomenonTime": "phenomenonTime"
}
```

`phenomenonTime` should be an ISO 8601 UTC string, e.g. `"2026-09-23T18:30:00Z"`. The library does not validate it.

---

## HydroServerMQTTClient

```cpp
#include <HydroServerMQTTClient.h>
```

A thin wrapper around [ArduinoMqttClient](https://github.com/arduino-libraries/ArduinoMqttClient) that publishes `Observation`s as SensorThings JSON and builds topics for you.

### Topic format

| Purpose | Topic |
| --- | --- |
| Observation | `<siteCode>/<clientId>/<sensorId>/<observedProperty>` |
| Last Will | `<siteCode>/<clientId>/lwt` |
| Subscription | `<siteCode>/<topic you pass to subscribe()>` |

Topics are limited to 255 characters (observations) and 127 characters (Last Will).

### Constants

| Name | Value | Meaning |
| --- | --- | --- |
| `HydroServerMQTTClient::defaultPort` | `1883` | Standard unencrypted MQTT port. |

### Defaults

| Setting | Default |
| --- | --- |
| Keep-alive interval | 60 s |
| Connection timeout | 20 s |
| Clean session | `false` (persistent session) |
| Subscribe QoS | 0 |
| Publish QoS | 0 |
| Last Will retain / QoS | `true` / 1 |

---

### Constructors

#### `HydroServerMQTTClient(Client &client, const char *broker, uint16_t port = 1883, const char *clientId = nullptr, const char *username = nullptr, const char *password = nullptr)`

Creates a client with its broker details up front. Only `client` and `broker` are required.

| Parameter | Description |
| --- | --- |
| `client` | Any object implementing Arduino's `Client` interface (`WiFiClient`, or `*modem.createClient()` on the Mayfly). Must outlive the MQTT client. |
| `broker` | Broker hostname or IP address. |
| `port` | Broker port. |
| `clientId` | MQTT client ID. Also used in topics. If omitted, ArduinoMqttClient generates a random ID on each connect — set one with `setClientID()` instead. |
| `username`, `password` | Optional broker login. Applied only if `username` is non-empty and `password` is non-null. |

```cpp
WiFiClient wifiClient;
HydroServerMQTTClient mqttClient(wifiClient, "test.mosquitto.org");
```

#### `HydroServerMQTTClient(Client &client)`

Creates a client with no broker set. Supply the broker later with `connectToBroker(broker, ...)`.

---

### Configuration

Call these **before** `connectToBroker()`.

#### `void setSiteCode(const char *siteCode)`

Sets the first segment of every topic. **Required** — without it, topics are malformed and the broker will drop the connection on publish. Use the site code from your HydroServer site ([site metadata guide](https://hydroserver.org/user-guides/how-to/managing-site-metadata.html#managing-site-metadata)).

#### `void setClientID(const char *clientId)`

Sets the MQTT client ID. Ignored if `nullptr` or empty. The ID should be unique on the broker and is used in topics, so a short lowercase string is recommended. A fixed ID is needed for persistent sessions.

#### `void setAuthentication(const char *username, const char *password)`

Sets broker credentials. Ignored unless `username` is non-empty and `password` is non-null (an empty password `""` is allowed).

#### `void setCleanSession(bool cleanSession)`

- `true` — the broker forgets this client's subscriptions and queued messages when it disconnects.
- `false` (default) — the broker keeps them for the next connection with the same client ID.

Only matters for QoS 1/2 messages. Set a client ID first.

#### `void setKeepAliveInterval(unsigned long seconds)`

How often the client pings the broker when idle. The broker disconnects the client after roughly 1.5× this interval of silence. Default 60 s.

#### `void setConnectionTimeout(unsigned long seconds)`

How long to wait for the network connection to the broker. Default 20 s.

#### `int setLastWill(const char *payload, bool retain = true, int qos = 1)`

Registers a Last Will message. If the client disappears without disconnecting cleanly (power loss, lost Wi-Fi), the broker publishes `payload` to `<siteCode>/<clientId>/lwt`. Call after `setSiteCode()`/`setClientID()` and before connecting.

| Parameter | Description |
| --- | --- |
| `payload` | Message text, e.g. `"offline"`. |
| `retain` | Keep the message on the broker for late subscribers. |
| `qos` | Delivery guarantee; at least 1 is recommended. |

**Returns** the result of ArduinoMqttClient's `endWill()`.

#### `const char *getLastWillTopic() const`

Returns the Last Will topic built by `setLastWill()`. Handy for printing or for publishing an "online" message to the same topic after connecting.

---

### Connection

#### `int connectToBroker()`

Connects using the settings already configured. Applies client ID, credentials, keep-alive, timeout and clean-session flag first.

**Returns** `0` if no broker is set or the connection fails; non-zero on success. On failure, call `getConnectionError()`.

```cpp
if (!mqttClient.connectToBroker()) {
  Serial.println(mqttClient.getConnectionError());
}
```

#### `int connectToBroker(const char *broker, uint16_t port = 1883, const char *clientId = nullptr, const char *username = nullptr, const char *password = nullptr)`

Sets the broker (and optionally the other details), then connects. `nullptr`/empty `clientId` or credentials leave previously set values unchanged.

#### `bool isConnected()`

`true` if the connection to the broker is open.

#### `int getConnectionError()`

The error code from the last connection attempt (ArduinoMqttClient's `connectError()`):

| Code | Meaning |
| --- | --- |
| `-2` | Connection refused (network level) |
| `-1` | Timed out |
| `0` | Success |
| `1` | Unacceptable protocol version |
| `2` | Client ID rejected |
| `3` | Server unavailable |
| `4` | Bad username or password |
| `5` | Not authorized |

#### `void poll()`

Sends keep-alive pings and delivers incoming messages. **Call it on every pass through `loop()`.** Without it, the broker eventually disconnects the client and `onMessage` callbacks never run.

---

### Publishing

#### `int publishObservation(const Observation &observation, const char *phenomenonTime)`

Publishes one observation to `<siteCode>/<clientId>/<sensorId>/<observedProperty>`.

**Returns** `0` if not connected. Otherwise the result of `endMessage()` — non-zero means the message was handed to the network. Publishing uses QoS 0, so this does **not** confirm the broker received it.

#### `void publishAll(Observation **observations, uint8_t size, const char *phenomenonTime)`

Publishes several observations with the same timestamp. Individual failures are not reported.

```cpp
Observation *all[] = {&temperature, &ph};
mqttClient.publishAll(all, 2, getISO8601Time());
```

#### `const char *getObservationTopic(const Observation &observation)`

Returns the topic an observation would be published to. The returned pointer refers to an internal buffer that is overwritten by the next call or publish — copy it if you need to keep it.

---

### Subscribing

#### `int subscribe(const char *topic)`

Subscribes to `<siteCode>/<topic>` at QoS 0.

**Returns** `0` if not connected; otherwise the result of the underlying subscribe.

```cpp
mqttClient.setSiteCode("uwrl");
mqttClient.subscribe("python_publisher/sensorid/temperature_celsius");
// subscribed to: uwrl/python_publisher/sensorid/temperature_celsius
```

#### `int unsubscribe(const char *topic)`

Unsubscribes from `topic`. Unlike `subscribe()`, the site code is **not** added — pass the full topic.

#### `void onMessage(void (*callback)(int))`

Registers a function to run whenever a message arrives on a subscribed topic. The callback receives the payload size in bytes and runs from inside `poll()`.

```cpp
void messageReceived(int size) {
  Serial.println(mqttClient.messageTopic());
  Serial.println(mqttClient.readMessage());
}
mqttClient.onMessage(messageReceived);
```

#### `String messageTopic()`

Topic of the message currently being handled.

#### `String readMessage()`

Reads and returns the whole payload of the current message. The payload can only be read once — use either `readMessage()` or `deserializePayload()`, not both.

#### `void deserializePayload()`

Parses the current message as an observation JSON payload and stores its `result` field. Call inside your `onMessage` callback.

#### `float getLatestValueofTopic(const char *topic)`

Returns the value stored by `deserializePayload()` if the current message's topic equals `<siteCode>/<topic>`. If the topic doesn't match, the return value is undefined — only call it for topics you know the message came from.

```cpp
void messageReceived(int size) {
  mqttClient.deserializePayload();
  Serial.println(mqttClient.getLatestValueofTopic("python_publisher/sensorid/temperature_celsius"));
}
```

---

## HydroServerHTTPClient

```cpp
#include <HydroServerHTTPClient.h>
```

Sends observations straight to a HydroServer instance through its SensorThings API, using [ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient). Each request sends these headers:

```
Accept: */*
X-Api-Key: <apiKey>
Content-Type: application/json
```

### Endpoints used

| Method | Path | Used by |
| --- | --- | --- |
| `POST` | `/api/sensorthings/v1.1/Observations` | `publishObservation()` |
| `GET` | `/api/sensorthings/v1.1/Datastreams('<id>')` | `isDatastreamAvailable()` |

---

### Constructor

#### `HydroServerHTTPClient(Client &client, const char *hydroServerURL = "playground.hydroserver.org", uint16_t port = 443, const char *apiKey = nullptr)`

| Parameter | Description |
| --- | --- |
| `client` | Network client. For port 443 it must be a TLS client (e.g. `WiFiSSLClient` on the Uno R4, `*modem.createSecureClient()` on the Mayfly ESP32). Must outlive the HTTP client. |
| `hydroServerURL` | HydroServer hostname, without `https://`. |
| `port` | `443` for HTTPS, `80` for plain HTTP. |
| `apiKey` | HydroServer API key with write access to your datastreams. Can also be set with `setApiKey()`. |

HTTP keep-alive is enabled, so the connection is reused between requests.

```cpp
WiFiSSLClient sslClient;
HydroServerHTTPClient hsClient(sslClient, "playground.hydroserver.org", 443, PLAYGROUND_API_KEY);
```

---

### Methods

#### `void setApiKey(const char *apiKey)`

Sets or replaces the API key. Ignored if `nullptr`.

#### `bool publishObservation(const Observation &observation, const char *phenomenonTime)`

POSTs one observation. **Returns** `true` only if the server responds with status `200`. Check `getLastResponseBody()` for the server's reply either way.

#### `void publishAll(Observation **observations, uint8_t size, const char *phenomenonTime)`

POSTs each observation in turn with the same timestamp. One request per observation; failures are not reported.

#### `bool isDatastreamAvailable(const char *datastreamId)`

**Returns** `true` if the datastream exists and is readable with your API key (status `200`). A good startup check to catch a mistyped UUID.

```cpp
if (!hsClient.isDatastreamAvailable(temperature.datastreamId)) {
  Serial.println(hsClient.getLastResponseBody());
}
```

#### `String getLastResponseBody() const`

The body of the most recent response — usually the error message when a request fails.

---

## Modem wrappers

```cpp
#include <modems/ExpressifESP32.h>   // or
#include <modems/DigiXbeeS6B.h>
```

Helpers for Wi-Fi Bee modules on the EnviroDIY Mayfly, built on [EnviroDIY's TinyGSM fork](https://github.com/EnviroDIY/TinyGSM). Include **only one** modem header per sketch — each one defines the TinyGSM modem type.

### Common interface (`Modem`)

| Method | Description |
| --- | --- |
| `void powerUp()` | Drives the power pin `HIGH` to switch on the Bee socket. |
| `bool connectToInternet(const char *ssid, const char *password, uint32_t maxConnectionTime = 60000)` | Powers up the modem, joins Wi-Fi and waits up to `maxConnectionTime` ms for an IP address. Returns `true` on success. |
| `bool isInternetAvailable()` | `true` if the modem has a network connection. |
| `uint32_t getNISTTime()` | Current UTC time as **seconds since 2000-01-01** (the DS3231 RTC's epoch), ready for `rtc.setDateTime()`. Add `946684800` to get Unix time. Returns `0` on failure. |
| `Client *createClient()` | New plain TCP client for MQTT or HTTP. |
| `Client *createSecureClient()` | New TLS client for HTTPS, or `nullptr` if the modem can't do TLS. |
| `void extraSetupForMQTT()` | Modem-specific setup needed before MQTT. Call after `connectToInternet()` and before `connectToBroker()`. |

`createClient()` and `createSecureClient()` allocate with `new`; create the client once (e.g. at global scope) rather than in `loop()`.

### `ExpressifESP32`

For an ESP32 Bee running Espressif AT firmware.

```cpp
ExpressifESP32(Stream &serial, int8_t powerPin);
ExpressifESP32(Stream &serial, int8_t powerPin, Stream &debugStream);
```

| Parameter | Description |
| --- | --- |
| `serial` | Serial port wired to the Bee (`Serial1` on the Mayfly). Start it at **57600** baud. |
| `powerPin` | Bee power pin (`18` on Mayfly 1.x). |
| `debugStream` | Optional. Echoes all AT traffic to this stream (e.g. `Serial`). |

ESP32-specific behaviour:

- `connectToInternet()` also initialises the modem.
- `getNISTTime()` syncs from `pool.ntp.org` (waits up to 30 s).
- `extraSetupForMQTT()` switches the socket to passive receive mode (`AT+CIPRECVMODE=1`).
- `createSecureClient()` returns a TLS client.

#### `void extraSetupForHTTPS(const char *host, int port)`

Required before HTTPS posts. Sets passive receive mode, sets the TLS Server Name Indication (SNI) to `host` — needed by servers such as the HydroServer playground that host several domains on one IP — and opens the SSL connection.

```cpp
modem.extraSetupForHTTPS("playground.hydroserver.org", 443);
```

### `DigiXbeeS6B`

For the Digi XBee S6B Wi-Fi Bee.

```cpp
DigiXbeeS6B(Stream &serial, int8_t powerPin);
DigiXbeeS6B(Stream &serial, int8_t powerPin, Stream &debugStream);
```

Parameters are the same as for `ExpressifESP32`.

XBee-specific behaviour:

- `createSecureClient()` returns `nullptr` — the S6B does not support TLS here, so use plain MQTT or HTTP.
- `getNISTTime()` uses the RFC 868 time protocol over TCP (port 37) to `time-a-wwv.nist.gov` (132.163.97.1), because the XBee can't hold UDP and TCP sockets open together. Returns `0` if there's no connection.
- `extraSetupForMQTT()` does nothing (nothing extra is needed), but calling it is harmless.
