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
### Quick Start for HydroServerMQTTClient
- Clone this github repo into `Arduino/libraries`.
- This library depends on ArduinoJson and TinyGsm. You can install ArduinoJson from Libary Manager from ArduinoIDE. Don't install TinyGSM from Library Manager as this uses forked version developed by EnviroDIY.

For testing the library, you can use free mosquitto broker available online `https://test.mosquitto.org/`.
Make sure to subscribe to the topic that you published to. Because this is commercially available, subscribing to all published topic using wildcard ("#") will result in getting all published message in the broker.
Look at `examples/Uno_mqtt_basic` for simple example on how to use HydroServerMQTTClient for publishing message to broker.

### Topic Structure
The library will automatically create topic for each observation and Last will based on the site id, client id and observation struct provided by the user. The topic for observation is in format 
```
    <sitecode>/<clientId>/<sensorId>/<observedProperty>/observations
```
Last will topic is in format 
```
    <sitecode>/<clientId>/lwt 
```