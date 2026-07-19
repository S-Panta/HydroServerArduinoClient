# HydroServerArduinoClient
An Arduino library for publishing sensor observations. It provides two publishers built on common DataPublisher interface:
- HydroServerHTTPClient - publishes observation directly to a HydroServer(https://github.com/hydroserver2) instance via HTTP POST
- HydroServerMQTTClient - publishes observation to a running MQTT broker. HydroServer can then consume this payload using python script. Note that this doesn't allow direct interaction with HydroServer. The client is built on top of ArduinoMQTTClient and wraps all functionality that library provides.


### Observation struct
```cpp
    struct Observation {
    const char *observedProperty;
    const char *datastreamId;
    const char *sensorId;
    double value = 0.0;
    };
```
|Field| Description|
| --- | --- |
| observedProperty | Name of observed property |
| datastreamId | HydroServer/SensorThings datastream UUID this observation belongs to |
| sensorId| Sensor identifier |
| value | observed numeric value |


### Payload format
Both client will publish the observation into same JSON format.
```json
    {
        "Datastream": 
            { "@iot.id": "<datastreamId>" },
        "result": <value>,
        "phenomenonTime": "<phenomenonTime>"
    }
```
### Installation
Clone the library and copy entire folder into `Arduino/libraries`.
For testing the library, you can use free mosquitto broker available online `https://test.mosquitto.org/`.
Make sure to subscribe to the topic that you published to. Because this is commercially available, subscribing to all published topic using wildcard ("#") will result in getting all published message in the broker.

### Quick Start for HydroServerMQTTClient
```cpp
#include <WiFiS3.h>
#include <HydroServerMQTTClient.h>

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;
const char* MQTT_BROKER = "test.mosquitto.org/";

Include the site code before you initialize your library
const char* SITE_CODE = "uwrl";
WiFiClient wifiClient;
HydroServerMQTTClient mqttClient(wifiClient, MQTT_BROKER);

Observation temperature = { "temperature", "uuidtemperature", "tempsensor" };

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    while(1);
  }
  Serial.println("\nWiFi connected!");
  // some time requires for DHCP to assign IP
  delay(5000);
  // IP address if not 0.0.0.0 means the device is connected to wifi
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  // connect to WiFi (see connectWiFi() in example sketch). 
  // The step to wifi connection differs from Arduino Uno r4 and Mayfly datalogger
  // This is only for arduino uno r4
  connectToWiFi();

  mqttClient.setClientID("Arduinopublisher");
  mqttClient.setLastWill("Shutting down the arduino");

  if (!mqttClient.connectToBroker()) {
    Serial.println(mqttClient.getConnectionError());
    while (1);
  }
}

void loop() {
  mqttClient.poll();
  temperature.value = 25.5;
  mqttClient.publishObservation(temperature, "2026-06-15T00:00:00Z");
  delay(10000);
}
```

### Topic Structure
The library will automatically create topic for each observation and Last will based on the site id, client id and observation struct provided by the user. The topic for observation is in format 
```
    <sitecode>/<clientId>/<sensorId>/<observedProperty>/observations
```
Last will topic is in format 
```
    <sitecode>/<clientId>/lwt 
```
Make sure to predefine site code using `const char* SITE_CODE = "uwrl";` in the beginning of your code before library initialization.

