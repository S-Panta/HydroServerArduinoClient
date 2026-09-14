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
- Install ArduinoJson, ArduinoMQTTClient and ArduinoHTTPClient from Libraries within IDE.
- Install TinyGsm(https://github.com/EnviroDIY/TinyGSM) and StreamDebugger(https://github.com/EnviroDIY/StreamDebugger) from the github. You can directly clone them into `Arduino/libraries` folder.
For testing the library, you can use free mosquitto broker available online `https://test.mosquitto.org/`.
Make sure to subscribe to the topic that you published to. Because this is commercially available, subscribing to all published topic using wildcard ("#") will result in getting all published message in the broker.
Look at `examples/UnoR4_Wifi/Uno_mqtt_basic.ino` for simple example on how to use HydroServerMQTTClient for publishing message to a running MQTT broker.
The phenomenonTime should be in Iso format. Therefore copy the function `getISO8601Time` from `Uno_mqtt_basic.ino`

### Topic Structure
The library will automatically create topic for each observation and Last will based on the site id, client id and observation struct provided by the user. The topic for observation is in format 
```
    <sitecode>/<clientId>/<sensorId>/<observedProperty>
```
Last will topic is in format 
```
    <sitecode>/<clientId>/lwt 
```
