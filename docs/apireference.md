# HydroServerMQTTClient API Reference

A lightweight Arduino wrapper on top of ArduinoMqttClient for publishing observation to Hydroserver. The observations are formatted according to OGC Sensorthing requirement.

## Constructors

### `HydroServerMQTTClient()`

Creates a new client bound to an existing network `Client` object
(e.g. from a modem or WiFi shield), with optional broker connection
details.

    HydroServerMQTTClient(Client& client, const char* broker, uint16_t port, const char* clientId, const char* username, const char* password)
    HydroServerMQTTClient(Client& client)

**Parameters**

- `client` — a reference to an already-configured network `Client`
  (e.g. from `DigiXBeeWifi`, a WiFi101 shield, or any class
  implementing the Arduino `Client` interface)
- `broker` — hostname or IP of the MQTT broker; may be `nullptr` and
  set later via `connectToBroker()`
- `port` — broker TCP port; defaults to `1883`
- `clientId` — MQTT client identifier; optional
- `username` — optional broker username
- `password` — optional broker password

The short-form constructor (`Client&` only) creates an uninitialised client instances. Before using this for mqtt, it must be configured with the setters by passing them either to `connectToBroker()` or the respective setters.

---

## Connection

### `connectToBroker()`

Connects to the broker using previously configured values.

    int connectToBroker()

Applies keep-alive interval, connection timeout, and clean-session
flag, then attempts the connection.

**Returns**

- `0` if no broker has been configured (`_broker == nullptr`)
- Otherwise, the result of the underlying `MqttClient::connect()`
  call — non-zero generally indicates success; use
  `getConnectionError()` for the specific failure code on failure

---

### `connectToBroker(broker, port, clientId, username, password)`

Sets broker connection details, then connects. This can be used when HydroServerMQTTClient is initialized only with client interface.

    int connectToBroker(const char* broker, uint16_t port, const char* clientId, const char* username, const char* password)

**Parameters**

- `broker` — hostname or IP of the MQTT broker
- `port` — broker TCP port
- `clientId` — MQTT client identifier; if `nullptr`, any previously
  set ID is retained
- `username` — optional broker username
- `password` — optional broker password

**Returns**

- Same as the no-argument `connectToBroker()`

**Note:** if you are calling this to change credentials on an
already-constructed client, verify the new credentials are actually
applied — as currently written, credential reassignment on this path
should be checked against the parameters passed in, not prior state.

---

### `isConnected()`

Checks whether the client currently has an active broker connection.

    bool isConnected()

**Returns**

- `true` if connected, `false` otherwise

---

### `getConnectionError()`

Retrieves the most recent connection error code.

    int getConnectionError()

**Returns**

- An error code as defined by `MqttClient::connectError()`; `0`
  generally indicates no error

---

## Configuration

### `setClientID()`

Sets the MQTT client identifier used on connect.

    void setClientID(const char* clientId)

**Parameters**

- `clientId` — the client ID string; ignored if `nullptr`

---

### `setAuthentication()`

Sets the username and password used on connect.

    void setAuthentication(const char* username, const char* password)

**Parameters**

- `username` — broker username
- `password` — broker password

Both must be non-null for either to be applied.

---

### `setCleanSession()`

Controls whether the broker discards client state on disconnect.

    void setCleanSession(bool cleanSession)

**Parameters**

- `cleanSession` — `true` starts a non-persistent session (broker
  discards state on disconnect); `false` requests the broker retain
  session state across reconnects. Only meaningful for QoS ≥ 1;
  irrelevant for QoS 0 messages.

---

### `setKeepAliveInterval()`

Sets the MQTT keep-alive interval.

    void setKeepAliveInterval(unsigned long seconds)

**Parameters**

- `seconds` — time between keep-alive pings sent to the broker

---

### `setConnectionTimeout()`

Sets how long to wait for the broker connection to establish.

    void setConnectionTimeout(unsigned long timeout)

**Parameters**

- `timeout` — timeout in seconds; not part of the MQTT spec itself,
  governs the underlying TCP/network connection attempt

---

## Publishing

### `publishObservation()`

Publishes a single observation, formatted per the OGC
SensorThings API.

    int publishObservation(const char* datastreamId, double observation, const char* phenomenonTime)

**Parameters**

- `datastreamId` — UUID of the target SensorThings Datastream
- `observation` — the measured value
- `phenomenonTime` — ISO 8601 timestamp of when the observation was
  taken

Builds a JSON payload (`phenomenonTime`, `result`,
`Datastream.@iot.id`) and publishes it to a topic derived from the
datastream ID.

**Returns**

- Result of the underlying publish call — non-zero generally
  indicates success

**Note:** confirm the topic path used matches the exact resource
naming your HydroServer SensorThings endpoint expects
(singular vs. plural collection name) — a mismatch here will not
raise an MQTT-level error, only a downstream ingestion failure.

---

### `publishAll()`

Publishes a batch of observations in one call.

    void publishAll(DataStream** datastreams, uint8_t count, const char* phenomenonTime)

**Parameters**

- `datastreams` — array of pointers to `DataStream` objects to
  publish
- `count` — number of entries in `datastreams`
- `phenomenonTime` — timestamp applied to all observations in this
  batch

Internally calls `publishObservation()` once per entry. Individual
publish failures are not surfaced — this method does not return a
status.

---

### `setLastWill()`

Registers a Last Will and Testament message with the broker.

    int setLastWill(const char* lastWillTopic, const char* payload)

**Parameters**

- `lastWillTopic` — topic the broker will publish to if this client
  disconnects ungracefully
- `payload` — the will message content

The broker publishes this message automatically if the client fails
to send a clean disconnect and the keep-alive interval elapses.

**Returns**

- Declared as `int`, but the current implementation does not return
  a value — treat as `void` until this is corrected, and do not rely
  on a return value to confirm the will was registered successfully

---

## Subscribing

### `subscribe()`

Subscribes to a topic.

    int subscribe(const char* topic)

**Parameters**

- `topic` — the MQTT topic to subscribe to

**Returns**

- `0` if not currently connected to the broker
- Otherwise, the result of the underlying subscribe call

**Note:** subscribes at QoS 0 by default; there is currently no
parameter to request QoS 1/2 on subscribe.

---

### `unsubscribe()`

Unsubscribes from a topic.

    int unsubscribe(const char* topic)

**Parameters**

- `topic` — the MQTT topic to unsubscribe from

**Returns**

- Result of the underlying unsubscribe call

---

### `onMessage()`

Registers a callback invoked when a message arrives on a subscribed
topic.

    void onMessage(void (*callback)(int))

**Parameters**

- `callback` — function pointer invoked with the size (in bytes) of
  the incoming message payload

---

### `poll()`

Processes incoming messages and maintains the connection.

    void poll()

Must be called regularly (e.g. in the main loop) for subscriptions
and keep-alive to function correctly.

---

### `readMessage()`

Reads the payload of the most recently received message.

    String readMessage()

**Returns**

- The message payload as a `String`, built by reading all currently
  available bytes

---

### `messageTopic()`

Gets the topic of the most recently received message.

    String messageTopic()

**Returns**

- The topic string of the current incoming message