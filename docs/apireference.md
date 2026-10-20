# API Reference

Reference for the public API of **HydroServerArduinoClient**. It describes what the code currently does, including limitations you need to work around.

- [Observation and payload](#observation-and-payload): `PublisherUtil.h`
- [HydroServerMQTTClient](#hydroservermqttclient): `HydroServerMQTTClient.h`
- [HydroServerHTTPClient](#hydroserverhttpclient): `HydroServerHTTPClient.h`
- [Modem wrappers](#modem-wrappers): `modems/modem.h`, `modems/ESP32Bee.h`, `modems/DigiXbeeS6B.h`

For installation and a walkthrough, see the [README](../README.md). For how the pieces fit together, see [architecture.md](architecture.md).

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
  double value;

  Observation(const char *observedProperty, const char *datastreamId,
              const char *sensorId, double value = NAN);
};
```

| Field | Description | Used in |
| --- | --- | --- |
| `observedProperty` | Name of what is measured, e.g. `"temperature"`. | MQTT topic (HTTP ignores it) |
| `datastreamId` | UUID of the HydroServer datastream the value belongs to. | Payload `Datastream.@iot.id` |
| `sensorId` | Your identifier for the physical sensor, e.g. `"temp-sensor-1"`. | MQTT topic (HTTP ignores it) |
| `value` | The measured value. `NAN` until you set it. | Payload `result` |

- There is no default constructor. `Observation obs;` does not compile; always pass the three strings.
- The struct stores **pointers**, not copies. Use string literals or global/static strings; a local `char` array goes out of scope and leaves the pointer dangling.
- Because topic segments are separated by `/`, don't put `/` in `observedProperty` or `sensorId`.

```cpp
Observation temperature = {"temperature", "019f246b-...", "temp-sensor-1"};
temperature.value = 21.7;
```

### `ObservationPayload::serialize()`

```cpp
String ObservationPayload::serialize(const Observation &observation, const char *phenomenonTime);
```

Builds the JSON body both clients send. You don't normally call it yourself.

```json
{
  "Datastream": { "@iot.id": "019f246b-..." },
  "result": 21.7,
  "phenomenonTime": "2026-09-23T18:30:00Z"
}
```

- `result` is a JSON number. An unset value (`NAN`) is serialized as `null`.
- `phenomenonTime` is copied as given. It must be an ISO 8601 UTC string such as `"2026-09-23T18:30:00Z"`; the library does not check the format.

---

## HydroServerMQTTClient

```cpp
#include <HydroServerMQTTClient.h>
```

A wrapper around [ArduinoMqttClient](https://github.com/arduino-libraries/ArduinoMqttClient) that publishes `Observation`s as SensorThings JSON and builds the topics for you.

### Topic format

| Purpose | Topic | Max length |
| --- | --- | --- |
| Observation | `<siteCode>/<clientId>/<sensorId>/<observedProperty>` | 255 characters |
| Last Will | `<siteCode>/<clientId>/lwt` | 127 characters |
| Subscription | `<siteCode>/<topic passed to subscribe()>` | 255 characters |

Longer topics are cut off at the limit without any error.

### Required setup

`setSiteCode()` and `setClientID()` have **no working defaults**. Call both before `setLastWill()`, `connectToBroker()`, `publishObservation()` or `subscribe()`. If either is missing, topics are built from a null pointer and the result is undefined (typically a garbled topic or a crash).

### Constants

| Name | Value | Meaning |
| --- | --- | --- |
| `HydroServerMQTTClient::defaultPort` | `1883` | Standard unencrypted MQTT port. |

### Defaults

| Setting | Default | Change with |
| --- | --- | --- |
| Port | 1883 | Constructor or `connectToBroker(broker, port)` |
| Keep-alive interval | 60 s | `setKeepAliveInterval()` |
| Connection timeout | 20 s | `setConnectionTimeout()` |
| Clean session | `false` (persistent session) | `setCleanSession()` |
| Publish QoS / retain | 0 / `false` | Not configurable |
| Subscribe QoS | 0 | Not configurable |
| Last Will retain / QoS | `true` / 1 | `setLastWill()` arguments |
| Username / password | none | `setAuthentication()` |

---

### Constructors

#### `HydroServerMQTTClient(Client &client, const char *broker, uint16_t port = 1883, const char *clientId = nullptr, const char *username = nullptr, const char *password = nullptr)`

Creates a client with its broker details up front. Only `client` and `broker` are needed here; the client ID can be set now or later with `setClientID()`.

| Parameter | Description |
| --- | --- |
| `client` | Any object implementing Arduino's `Client` interface (`WiFiClient` on the Uno R4, `*modem.createClient()` on the Mayfly). It must exist for as long as the MQTT client does. |
| `broker` | Broker hostname or IP address. The string is not copied. |
| `port` | Broker port. |
| `clientId` | MQTT client ID, also used in topics. Same rules as `setClientID()`. |
| `username`, `password` | Broker login. Same rules as `setAuthentication()`. |

```cpp
WiFiClient wifiClient;
HydroServerMQTTClient mqttClient(wifiClient, "test.mosquitto.org");
```

#### `HydroServerMQTTClient(Client &client)`

Creates a client with no broker. You must then connect with `connectToBroker(broker, ...)`; the no-argument `connectToBroker()` returns `0`.

---

### Configuration

Call these **before** `connectToBroker()`. They store settings only; nothing is sent to the broker until you connect.

#### `void setSiteCode(const char *siteCode)`

Sets the first segment of every topic. **Required** (see [Required setup](#required-setup)). Ignored if `siteCode` is `nullptr` or empty. Use the site code of your HydroServer site ([site metadata guide](https://hydroserver.org/user-guides/how-to/managing-site-metadata.html#managing-site-metadata)).

#### `void setClientID(const char *clientId)`

Sets the MQTT client ID, which is also the second segment of every topic. **Required** (see [Required setup](#required-setup)). Ignored if `clientId` is `nullptr` or empty.

The ID must be unique on the broker: if two devices connect with the same ID, the broker disconnects the first one. Topics are case-sensitive, so a short lowercase ID such as `"mayfly-01"` is recommended. A fixed ID is also what lets the broker keep a persistent session for this device.

#### `void setAuthentication(const char *username, const char *password)`

Sets the broker login. Stored only if `username` is non-empty and `password` is not `nullptr`; otherwise the call does nothing. An empty password `""` is accepted.

#### `void setCleanSession(bool cleanSession)`

Controls what the broker remembers about this client between connections.

- `true`: the broker discards this client's subscriptions and any queued messages when it disconnects.
- `false` (default): the broker keeps them for the next connection with the same client ID.

Since this library publishes and subscribes at QoS 0, the broker queues no messages for it; in practice this setting only decides whether subscriptions survive a reconnect.

#### `void setKeepAliveInterval(unsigned long seconds)`

Sets the keep-alive interval in **seconds**. If the broker hears nothing from the client for about 1.5 × this interval, it disconnects the client. `poll()` sends the keep-alive pings, so set this longer than the longest gap between `poll()` calls (for example, longer than the logging interval on a logger that sleeps).

The MQTT maximum is 65535 s. Larger values are not rejected but wrap around to a much smaller interval, so stay below 65535.

#### `void setConnectionTimeout(unsigned long seconds)`

How long, in **seconds**, `connectToBroker()` waits for the broker to accept the connection before giving up. Default 20 s.

#### `int setLastWill(const char *payload, bool retain = true, int qos = 1)`

Registers a Last Will message. If the client drops off without disconnecting cleanly (power loss, lost Wi-Fi), the broker publishes `payload` to `<siteCode>/<clientId>/lwt`.

The topic is built when you call this function, so call it **after** `setSiteCode()` and `setClientID()` and **before** `connectToBroker()`.

| Parameter | Description |
| --- | --- |
| `payload` | Message text, e.g. `"offline"`. |
| `retain` | `true` keeps the message on the broker so clients that subscribe later still receive it. |
| `qos` | 0, 1 or 2. Use at least 1 so the broker reliably delivers it. |

**Returns** a non-zero value if the message was stored, `0` otherwise.

#### `const char *getLastWillTopic() const`

Returns the Last Will topic built by the last `setLastWill()` call, e.g. to print it or to publish an `"online"` message there after connecting. Only meaningful after `setLastWill()` has been called.

---

### Connection

#### `int connectToBroker()`

Applies the client ID, credentials, keep-alive, connection timeout and clean-session setting, then opens the connection to the broker set in the constructor (or by an earlier `connectToBroker(broker, ...)` call).

**Returns** `1` on success, `0` if no broker is set or the connection fails. On failure, `getConnectionError()` gives the reason.

```cpp
if (!mqttClient.connectToBroker()) {
  Serial.println(mqttClient.getConnectionError());
}
```

#### `int connectToBroker(const char *broker, uint16_t port = 1883, const char *clientId = nullptr, const char *username = nullptr, const char *password = nullptr)`

Sets the broker and port, optionally the client ID and login, then calls `connectToBroker()`.

- `broker` and `port` always replace the stored values, so omitting `port` resets it to 1883.
- A `nullptr` or empty `clientId`, or a `username`/`password` pair that `setAuthentication()` would ignore, leaves the stored value unchanged.

**Returns** the same as `connectToBroker()`.

#### `bool isConnected()`

`true` if the network connection to the broker is open.

#### `int getConnectionError()`

The result of the last connection attempt:

| Code | Meaning |
| --- | --- |
| `-2` | Network connection refused: the broker host or port couldn't be reached. |
| `-1` | Timed out waiting for the broker. |
| `0` | Success. |
| `1` | Broker refused: unacceptable protocol version. |
| `2` | Broker refused: client ID rejected. |
| `3` | Broker refused: server unavailable. |
| `4` | Broker refused: bad username or password. |
| `5` | Broker refused: not authorized. |

#### `void poll()`

Sends keep-alive pings and delivers incoming messages to the `onMessage()` callback. **Call it on every pass through `loop()`.** Without it, the broker eventually disconnects the client and the callback never runs.

---

### Publishing

#### `int publishObservation(const Observation &observation, const char *phenomenonTime)`

Publishes one observation, with QoS 0 and retain off, to `<siteCode>/<clientId>/<sensorId>/<observedProperty>`.

**Returns** `0` if the client is not connected, otherwise `1` once the message has been written to the network. At QoS 0 the broker sends no acknowledgement, so `1` does **not** confirm the broker received it.

#### `uint8_t publishAll(Observation **observations, uint8_t size, const char *phenomenonTime)`

Publishes each observation in the array, all with the same timestamp, by calling `publishObservation()` for each.

**Returns** how many of them `publishObservation()` reported as written. Compare it with `size` to detect a failure.

```cpp
Observation *all[] = {&temperature, &ph};
uint8_t published = mqttClient.publishAll(all, 2, getISO8601Time());
```

#### `const char *getObservationTopic(const Observation &observation)`

Returns the topic the observation would be published to. The pointer refers to an internal buffer that the next call to this function or to `publishObservation()` overwrites; copy the string if you need to keep it.

---

### Subscribing

Incoming messages are handled inside `poll()`. The usual pattern is: connect, register a callback with `onMessage()`, then `subscribe()`.

#### `int subscribe(const char *topic)`

Subscribes, at QoS 0, to `<siteCode>/<topic>`. The site code is added for you.

**Returns** `0` if the client is not connected, otherwise non-zero if the subscribe request was sent.

```cpp
mqttClient.setSiteCode("uwrl");
mqttClient.subscribe("python_publisher/sensorid/temperature_celsius");
// subscribed to: uwrl/python_publisher/sensorid/temperature_celsius
```

#### `int unsubscribe(const char *topic)`

Unsubscribes from `topic`. Unlike `subscribe()`, the site code is **not** added, so pass the full topic, e.g. `"uwrl/python_publisher/sensorid/temperature_celsius"`.

**Returns** non-zero if the request was sent.

#### `void onMessage(void (*callback)(int))`

Registers the function to call when a message arrives on any subscribed topic. It is called from inside `poll()` with the payload size in bytes. Inside the callback, use `messageTopic()` to see which topic it came from, then read the payload once with either `readMessage()` or `deserializePayload()`.

```cpp
void messageReceived(int size) {
  Serial.println(mqttClient.messageTopic());
  Serial.println(mqttClient.readMessage());
}
mqttClient.onMessage(messageReceived);
```

#### `String messageTopic()`

The full topic of the message currently being handled, including the site code. Only meaningful inside the `onMessage()` callback.

#### `String readMessage()`

Reads and returns the whole payload of the current message as text. The payload can only be read once, so don't also call `deserializePayload()` for the same message.

#### `void deserializePayload()`

Reads the current message, parses it as an observation JSON payload, and stores its `result` field as a `float` for `getLatestValueofTopic()`. Call it inside the `onMessage()` callback, at most once per message.

- Only one value is stored; each call replaces it, whatever the topic.
- If the payload isn't valid JSON or has no numeric `result`, no error is reported and the stored value becomes `0`.

#### `float getLatestValueofTopic(const char *topic)`

Returns the value stored by the last `deserializePayload()` call, **provided** the current message's topic is exactly `<siteCode>/<topic>`.

Only call it inside the `onMessage()` callback, after `deserializePayload()`, and only with the topic the message actually arrived on. Check that first with `messageTopic()`. If the topic doesn't match, the return value is undefined (garbage). The full topic must be 127 characters or fewer.

```cpp
const char *TEMP_TOPIC = "python_publisher/sensorid/temperature_celsius";

void messageReceived(int size) {
  String topic = mqttClient.messageTopic();
  mqttClient.deserializePayload();
  if (topic.endsWith(TEMP_TOPIC)) {
    Serial.println(mqttClient.getLatestValueofTopic(TEMP_TOPIC));
  }
}
```

---

## HydroServerHTTPClient

```cpp
#include <HydroServerHTTPClient.h>
```

Sends observations straight to a HydroServer instance through its SensorThings API, using [ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient). Every request sends these headers:

```
Accept: */*
X-Api-Key: <apiKey>
Content-Type: application/json
```

`Content-Length` is added when the request has a body.

### Endpoints used

| Method | Path | Used by |
| --- | --- | --- |
| `POST` | `/api/sensorthings/v1.1/Observations` | `publishObservation()`, `publishAll()` |
| `GET` | `/api/sensorthings/v1.1/Datastreams('<id>')` | `isDatastreamAvailable()` |

---

### Constructor

#### `HydroServerHTTPClient(Client &client, const char *hydroServerURL = "playground.hydroserver.org", uint16_t port = 443, const char *apiKey = nullptr)`

| Parameter | Description |
| --- | --- |
| `client` | Network client. Port 443 needs a TLS client: `WiFiSSLClient` on the Uno R4, `*modem.createSecureClient()` on the Mayfly ESP32. Port 80 uses a plain client. It must exist for as long as the HTTP client does. |
| `hydroServerURL` | Host name or IP address only, without `https://` or a path, e.g. `"playground.hydroserver.org"`. |
| `port` | `443` for HTTPS, `80` for plain HTTP. |
| `apiKey` | HydroServer API key with write access to your datastreams. |

**You must provide an API key** here or with `setApiKey()` before the first request. If you don't, the request is sent with an invalid `X-Api-Key` header and the behaviour is undefined.

HTTP keep-alive is enabled, so the connection is reused between requests.

```cpp
WiFiSSLClient sslClient;
HydroServerHTTPClient hsClient(sslClient, "playground.hydroserver.org", 443, PLAYGROUND_API_KEY);
```

---

### Methods

#### `void setApiKey(const char *apiKey)`

Sets or replaces the API key. Ignored if `apiKey` is `nullptr`. The string is not copied, so it must stay valid (a `#define` or a global is fine).

#### `bool publishObservation(const Observation &observation, const char *phenomenonTime)`

POSTs one observation and waits for the server's reply.

**Returns** `true` only if the server answers with status code **exactly `200`**. Any other result returns `false`: other success codes such as `201 Created`, client and server errors (`4xx`, `5xx`), and network failures. In every case `getLastResponseBody()` holds what the server sent back; check it to tell the cases apart.

#### `void publishAll(Observation **observations, uint8_t size, const char *phenomenonTime)`

POSTs each observation in turn, one request each, all with the same timestamp. Nothing is returned, so failures are not reported; afterwards `getLastResponseBody()` only reflects the **last** request. To check each one, call `publishObservation()` in your own loop instead.

#### `bool isDatastreamAvailable(const char *datastreamId)`

Requests the datastream from the server. **Returns** `true` only if the server answers `200`, meaning the datastream exists and your API key can read it. Use it at startup to catch a mistyped UUID or a key without access.

```cpp
if (!hsClient.isDatastreamAvailable(temperature.datastreamId)) {
  Serial.println(hsClient.getLastResponseBody());
}
```

#### `String getLastResponseBody() const`

The body of the response to the most recent request, from either `publishObservation()` or `isDatastreamAvailable()`. After a failed request this usually holds the server's error message. Empty before the first request.

---

## Modem wrappers

```cpp
#include <modems/ESP32Bee.h>   // or
#include <modems/DigiXbeeS6B.h>
```

Helpers for Wi-Fi Bee modules on the EnviroDIY Mayfly, built on [EnviroDIY's TinyGSM fork](https://github.com/EnviroDIY/TinyGSM).

- Include **only one** modem header per sketch. Each one selects a different TinyGSM modem type, and only the first one included takes effect, so including both gives a modem that doesn't work.
- The modem source files are compiled with the library on every board, so the TinyGSM fork and `StreamDebugger` must be installed even if you don't use a modem.

### Common interface (`Modem`)

| Method | Description |
| --- | --- |
| `void powerUp()` | Sets the power pin to output and drives it `HIGH`, switching on the Bee socket. There is no matching power-down method. |
| `bool connectToInternet(const char *ssid, const char *password, uint32_t maxConnectionTime = 60000)` | Calls `powerUp()`, joins the Wi-Fi network, then waits up to `maxConnectionTime` **milliseconds** for the network connection. Returns `true` when connected, `false` if joining fails or the wait times out. Pass `0` (or `nullptr`) as the password for an open network. |
| `bool isInternetAvailable()` | `true` if the modem is connected to the Wi-Fi network. It does not test whether a particular server is reachable. |
| `uint32_t getNISTTime()` | Fetches the current UTC time and returns it as **seconds since 2000-01-01**, the epoch of the Mayfly's DS3231 clock, ready for `rtc.setDateTime()`. Add `946684800` for Unix time. Failure behaviour differs by modem; see below. Call it once after connecting, not on every reading. |
| `Client *createClient()` | Creates a plain TCP client for MQTT or HTTP. |
| `Client *createSecureClient()` | Creates a TLS client for HTTPS, or returns `nullptr` if the modem can't do TLS. |
| `void extraSetupForMQTT()` | Modem-specific setup needed before opening TCP connections. Call after `connectToInternet()` and before `connectToBroker()` or the first plain-HTTP request. |

`createClient()` and `createSecureClient()` allocate a new client with `new` on every call, and nothing frees it. Call each one **once**, normally at global scope when constructing the MQTT or HTTP client, never in `loop()`:

```cpp
HydroServerMQTTClient mqttClient(*modem.createClient(), MQTT_BROKER);
```

### `ESP32Bee`

For an ESP32 Bee running Espressif AT firmware.

```cpp
ESP32Bee(Stream &serial, int8_t powerPin);
ESP32Bee(Stream &serial, int8_t powerPin, Stream &debugStream);
```

| Parameter | Description |
| --- | --- |
| `serial` | Serial port wired to the Bee (`Serial1` on the Mayfly). Call `begin(57600)` on it before using the modem. |
| `powerPin` | Bee power pin (`18` on Mayfly v1.x). |
| `debugStream` | Optional. Copies all AT traffic to this stream (e.g. `Serial`) for debugging. |

ESP32-specific behaviour:

- `connectToInternet()` also initialises the modem before joining Wi-Fi.
- `getNISTTime()` syncs from `pool.ntp.org` and waits up to 30 s. **It does not return `0` on failure**; if the sync fails the result is a wrong (very large) number. Check that the year looks right after setting the clock.
- `extraSetupForMQTT()` switches the modem to passive receive mode (`AT+CIPRECVMODE=1`). Despite the name, plain HTTP needs it too.
- `createSecureClient()` returns a TLS client.

#### `void extraSetupForHTTPS(const char *host, int port)`

Required once before the first HTTPS request; call it after `connectToInternet()`. It:

1. switches to passive receive mode (as `extraSetupForMQTT()` does),
2. sets the TLS Server Name Indication (SNI) to `host`, which servers such as the HydroServer playground need because they host several domains on one IP address,
3. opens the SSL connection to `host`:`port`.

It takes a few seconds. You don't need to call `extraSetupForMQTT()` as well.

```cpp
modem.extraSetupForHTTPS("playground.hydroserver.org", 443);
```

### `DigiXbeeS6B`

For the Digi XBee S6B Wi-Fi Bee.

```cpp
DigiXbeeS6B(Stream &serial, int8_t powerPin);
DigiXbeeS6B(Stream &serial, int8_t powerPin, Stream &debugStream);
```

Parameters are the same as for `ESP32Bee`, except that the XBee S6B's serial port normally runs at **9600** baud.

XBee-specific behaviour:

- `createSecureClient()` returns `nullptr`; TLS isn't supported, so use plain MQTT or HTTP.
- `getNISTTime()` gets the time over TCP (RFC 868 time protocol, port 37) from `time-a-wwv.nist.gov` (132.163.97.1), because the XBee can't keep UDP and TCP sockets open at the same time. It returns `0` if the modem isn't connected or the time server can't be reached. If the server accepts the connection but doesn't answer within 5 s, the return value is undefined. Each call also allocates a TCP client that is never freed, which is another reason to call it only once.
- `extraSetupForMQTT()` does nothing; calling it is harmless.
