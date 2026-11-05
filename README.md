# HydroServerArduinoClient
HydroServerArduinoClient is a client wrapper built on top of the ![ArduinoMqttClient](https://github.com/arduino-libraries/ArduinoMqttClient) and ![ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient) libraries. It provides a simple interface for sending sensor observations from Arduino-based field loggers to [HydroServer](https://github.com/hydroserver2) over HTTP(S) or to broker via MQTT.


- **`HydroServerHTTPClient`** posts straight to HydroServer's SensorThings API.
- **`HydroServerMQTTClient`** publishes to any MQTT broker.

## Installation

Clone the library into your Arduino `libraries` folder:

```bash
cd ~/Documents/Arduino/libraries   
git clone https://github.com/S-Panta/HydroServerArduinoClient.git
```

Then install `ArduinoJson` (v7 or later), `ArduinoMqttClient`, `ArduinoHttpClient` and `StreamDebugger` from the Arduino Library Manager.

Install TinyGSM package developed by EnviroDIY. The modems driver inside `modems` is written on top of this library.

```bash
cd ~/Documents/Arduino/libraries
git clone https://github.com/EnviroDIY/TinyGSM.git
```


## Some concept

### Observations

An `Observation` holds one reading:

```cpp
struct Observation {
  const char *observedProperty;  // what is measured, e.g. "temperature"
  const char *datastreamId;      // HydroServer datastream UUID
  const char *sensorId;          // your name for the physical sensor
  double value;                  // the reading (NAN until you set it)
};

Observation temperature = {"temperature", "<datastream-uuid>", "temp-sensor-1"};
temperature.value = 21.7;
```

Both clients turn it into this JSON:

```json
{
  "Datastream": { "@iot.id": "<datastreamId>" },
  "result": 21.7,
  "phenomenonTime": "2026-09-23T18:30:00Z"
}
```

### Timestamps

Every Arduino board keeps time differently. Pass the timestamp into each publish call as a UTC string in ISO 8601 format, like `2026-09-23T18:30:00Z`.

On the **Uno R4 WiFi**, set the on-board RTC from an NTP server. The `UnoR4_Wifi` examples have `syncRTCFromNTP()` and `getISO8601Time()` helpers you can copy.

On the **Mayfly**, set the DS3231 clock with `rtc.setDateTime(modem.getNISTTime())`, then format it with the `getISO8601Time()` helper from the `mayfly_esp32` examples. If you use a ModularSensors logger, `Logger::formatDateTime_ISO8601(Logger::markedLocalUnixTime)` does the formatting for you.

### MQTT topics

Observations are published to `<siteCode>/<clientId>/<sensorId>/<observedProperty>`. If you set a last will message, it goes to `<siteCode>/<clientId>/lwt`.

Because the site code and client ID are part of every topic, call `setSiteCode()` and `setClientID()` before connecting. Use your HydroServer site's code (see [managing site metadata](https://hydroserver.org/user-guides/how-to/managing-site-metadata.html#managing-site-metadata)). Topics are case-sensitive, so stick to lowercase IDs and property names.

## Quick start: MQTT

This Uno R4 WiFi sketch publishes a reading every 30 seconds. The full version, with the clock helpers, is in `examples/UnoR4_Wifi/mqtt_basic`.

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

  syncRTCFromNTP();                      

  mqttClient.setSiteCode("mysite");        
  mqttClient.setClientID("logger01");      
  mqttClient.setLastWill("offline");      

  if (!mqttClient.connectToBroker()) {
    Serial.print("MQTT connect failed, error: ");
    Serial.println(mqttClient.getConnectionError());
    while (true);
  }
}

void loop() {
  mqttClient.poll();                       

  static unsigned long last = 0;
  if (millis() - last > 30000) {
    last = millis();
    temperature.value = analogRead(A0) * 0.1;   // replace with a real reading
    mqttClient.publishObservation(temperature, getISO8601Time());
  }
}
```

`poll()` has to run on every pass through `loop()`. 

## Quick start: HTTP to HydroServer

`HydroServerHTTPClient` posts each observation to `/api/sensorthings/v1.1/Observations` on your HydroServer instance, with your API key in the `X-Api-Key` header. The full version is in `examples/UnoR4_Wifi/http_post_hydroserver`.

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
  syncRTCFromNTP();                     

  // Catches a mistyped UUID or a key without access before deployment
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

You can pass the API key to the constructor or set it later with `setApiKey()`.

## Using the Mayfly modem wrappers

On the Mayfly, the Wi-Fi Bee is controlled with AT commands over `Serial1`. See the example on how to work with your Mayfly and ESP32Bee. If you are not sure about the baud rate for Esp32, call `detectBaud()` function. The baud rate for XbeeS6B is always 9600

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
  // ...then set up and connect mqttClient as in the MQTT quick start
}
```

For HTTPS on the ESP32, use `*modem.createSecureClient()` and call `modem.extraSetupForHTTPS(host, 443)` instead of `extraSetupForMQTT()`.

The XBee S6B can't do TLS, so use plain MQTT or HTTP with it.The last two also need the sensor drivers described below.
See the example folders for different use cases.

## SDI-12 sensor drivers for the Mayfly

`extra/sensors/` has [ModularSensors](https://github.com/EnviroDIY/ModularSensors) drivers for two SDI-12 sensors that ModularSensors doesn't support yet: the FTS DTS-12 turbidity sensor (`DTS12.h`, `DTS12.cpp`) and the Campbell Scientific CS451 pressure transducer (`CS451.h`, `CS451.cpp`).

They aren't compiled with this library, because they're built on ModularSensors' own sensor classes. To use them, copy the files into `ModularSensors/src/sensors/`, add the SDI-12 settings they need to `ModSensorConfig.h`, and include them in your sketch:

```cpp
#include <sensors/DTS12.h>
#include <sensors/CS451.h>
```

The [sensor drivers README](https://github.com/S-Panta/HydroServerArduinoClient/blob/main/extra/sensors/README.md) has the configuration lines and the variables each sensor reports.