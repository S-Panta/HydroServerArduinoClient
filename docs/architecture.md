# HydroServerArduinoClient — Class Diagram

This diagram shows the class hierarchy for the HydroServerArduinoClient library:
`DataPublisher` is the abstract base class, implemented by both
`HydroServerHTTPClient` and `HydroServerMQTTClient`, each of which uses the
`Observation` struct to build and publish sensor data.

```mermaid
classDiagram
    class Observation {
        +observedProperty : const char*
        +datastreamId : const char*
        +sensorId : const char*
        +value : double
    }

    class DataPublisher {
        <<abstract>>
        #createObservationPayload(observation, phenomenonTime) String$
        +publishObservation(observation, phenomenonTime) int*
        +publishAll(observations, count, phenomenonTime) void
    }

    class HydroServerHTTPClient {
        -_httpClient : HttpClient
        -_apiKey : const char*
        -_apiPath : const char*
        -_responseBody : String
        +HydroServerHTTPClient(client, hydroServerURL, port, apiKey)
        +setApiKey(apiKey) void
        +publishObservation(observation, phenomenonTime) int
        +getResponseBody() String
    }

    class HydroServerMQTTClient {
        -_mqttClient : MqttClient
        -_broker : const char*
        -_port : uint16_t
        -_clientId : const char*
        -_username : const char*
        -_password : const char*
        -_keepAliveSeconds : unsigned long
        -_connectionTimeout : unsigned long
        -_cleanSession : bool
        -_sitecode : const char*
        +defaultPort : uint16_t$
        +HydroServerMQTTClient(client, broker, port, clientId, username, password)
        +HydroServerMQTTClient(client)
        +setSiteCode(siteCode) void
        +setAuthentication(username, password) void
        +setClientID(clientId) void
        +setLastWill(payload) int
        +setCleanSession(cleanSession) void
        +setKeepAliveInterval(seconds) void
        +setConnectionTimeout(seconds) void
        +connectToBroker(broker, port, clientId, username, password) int
        +connectToBroker() int
        +isConnected() bool
        +getConnectionError() int
        +subscribe(topic) int
        +unsubscribe(topic) int
        +poll() void
        +onMessage(callback) void
        +readMessage() String
        +messageTopic() String
        +publishObservation(observation, phenomenonTime) int
    }

    DataPublisher <|-- HydroServerHTTPClient
    DataPublisher <|-- HydroServerMQTTClient
    DataPublisher ..> Observation : uses
```
