/*
  This file is part of HydroServerArduinoClient library.
  Sabin Panta
*/

#include "HydroServerMQTTClient.h"

// primary constructor where all parameter are given
HydroServerMQTTClient::HydroServerMQTTClient(Client &client, const char *broker,
                                             uint16_t port,
                                             const char *clientId,
                                             const char *username,
                                             const char *password)
    : // constructor initializer list where each class variable are
      // assigned their initialize value here
      _mqttClient(client), _port(port), _broker(broker),
      // These arguments are optional and therefore assigned nullptr
      _clientId(nullptr), _username(nullptr), _password(nullptr),
      _sitecode(nullptr) {

  setClientID(clientId);
  setAuthentication(username, password);
}

HydroServerMQTTClient::HydroServerMQTTClient(Client &client)
    : HydroServerMQTTClient(client, nullptr) {}

void HydroServerMQTTClient::setCleanSession(bool cleanSession) {
  // Setting clean session to true starts a non-persistent session. When a
  // client disconnects, the MQTT broker completely discards its state. For
  // publisher, this may not be important. Setting clean session to false, the
  // MQTT broker will remember the client state. A randomized or auto-generated
  // Client ID would be always mean clean session For QoS 0 messages, session
  // type is irrelevant
  _cleanSession = cleanSession;
}

int HydroServerMQTTClient::connectToBroker() {
  // This function takes parameter from the constructor itself
  // or if set by connectToBroker methods

  if (_broker == nullptr || strlen(_broker) == 0) {
    return 0;
  }
  // mqtt connect config
  _mqttClient.setId(_clientId);
  _mqttClient.setUsernamePassword(_username, _password);
  _mqttClient.setKeepAliveInterval(_keepAliveSeconds * 1000UL);
  _mqttClient.setConnectionTimeout(_connectionTimeout * 1000UL);
  _mqttClient.setCleanSession(_cleanSession);

  return _mqttClient.connect(_broker, _port);
}

int HydroServerMQTTClient::connectToBroker(const char *broker, uint16_t port,
                                           const char *clientId,
                                           const char *username,
                                           const char *password) {

  _broker = broker;
  _port = port;

  setClientID(clientId);
  setAuthentication(username, password);
  return connectToBroker();
}

void HydroServerMQTTClient::setClientID(const char *clientId) {
  // if clientid is not provided, the ArduinoMqttlibrary randomly generates a
  // client id in form of Arduino-millis() where millis is the time when device
  // was connected to the server Therefore,user should provide unique client id
  // for persistent session
  if (clientId != nullptr && strlen(clientId) > 0) {
    _clientId = clientId;
  }
}

void HydroServerMQTTClient::setSiteCode(const char *siteCode) {
  _sitecode = siteCode;
}

void HydroServerMQTTClient::setAuthentication(const char *username,
                                              const char *password) {
  // some MQTT broker allows password field to be empty
  if (username != nullptr && strlen(username) > 0 && password != nullptr) {
    _username = username;
    _password = password;
  }
}

int HydroServerMQTTClient::publishObservation(const Observation &observation,
                                              const char *phenomenonTime) {
  // MQTT-3.1.2-23 : It is the responsibility of the Client to ensure that the
  // interval between Control Packets being sent does not exceed the Keep Alive
  // value .In the absence of sending any other Control Packets, the Client MUST
  // send a PINGREQ Packet

  String payload = createObservationPayload(observation, phenomenonTime);

  if (!_mqttClient.connected()) {
    // checks the tcp client. So if broker is disconnected,this will notify
    return 0;
  };
  // to:do setup retain and dup and qos
  // if retain is true, broker logs change from r0 to r1. by default retain =
  // true beginMessage(topic, size, retain, qos, dup); to-do : Since default
  // payload limit is 256, research on how that be increased

  // topic construction is of format:
  // “sitecode/deviceid/sensorid/observedproperty
  char topic[256];
  // if all values of this topic is not initalized, broker will response with
  // malformed packet and mqtt connection will be terminated the first part
  // _sitecode should be valid, if not valid, no topic will form. other value
  // can be empty and mqtt works but not recommended
  snprintf(topic, sizeof(topic), "%s/%s/%s/%s", _sitecode, _clientId,
           observation.sensorId, observation.observedProperty);
  _mqttClient.beginMessage(topic, (unsigned long)payload.length());
  _mqttClient.print(payload);
  // only under qos 1 and 2 will return code from endmessage be 0 when message
  // sending fails Under qos 0, return code will remain 1 which could be
  // misleading
  return _mqttClient.endMessage();
}

int HydroServerMQTTClient::setLastWill(const char *payload) {
  size_t payloadLength = strlen(payload);
  char lastWillTopic[128];
  snprintf(lastWillTopic, sizeof(lastWillTopic), "%s/%s/lwt", _sitecode,
           _clientId);
  // to do : implement retain and qos
  _mqttClient.beginWill(lastWillTopic, payloadLength, true, 1);
  _mqttClient.print(payload);
  return _mqttClient.endWill();
}

int HydroServerMQTTClient::getConnectionError() {
  return _mqttClient.connectError();
}

void HydroServerMQTTClient::setKeepAliveInterval(unsigned long seconds) {
  _keepAliveSeconds = seconds;
}
void HydroServerMQTTClient::setConnectionTimeout(unsigned long timeout) {
  // This is not part of mqtt connect
  // The time for which client will wait for the network connection to the MQTT
  // server to be established. By default it is 30 seconds
  _connectionTimeout = timeout;
}

int HydroServerMQTTClient::subscribe(const char *topic) {
  if (!_mqttClient.connected())
    return 0;
  char fullTopic[256];
  snprintf(fullTopic, sizeof(fullTopic), "%s/%s", _sitecode, topic);
  // by default, qos will be 0
  return _mqttClient.subscribe(fullTopic);
}

void HydroServerMQTTClient::deserializePayload() {
  JsonDocument doc;
  deserializeJson(doc, _mqttClient);

  _latestValue = (float)doc["result"];
}

float HydroServerMQTTClient::getLatestValueofTopic(const char *topic) {
  char fullTopic[128];
  snprintf(fullTopic, sizeof(fullTopic), "%s/%s", _sitecode, topic);

  if (strcmp(fullTopic, messageTopic().c_str()) == 0) {
    Serial.println("Am I here");
    return _latestValue;
  }
}

// Registers the given callback to run whenever an MQTT message arrives,
// on any topic the client is currently subscribed to.
// If the client isn't subscribed to a topic, or the broker never
// publishes to it, the callback simply won't run for that topic.
// define a function that you want to do to incoming payload and register to
// this function as callback Example : void doSomething(){};
// mqttclient.onMessage(doSomething)
void HydroServerMQTTClient::onMessage(void (*callback)(int)) {
  _mqttClient.onMessage(callback);
}

int HydroServerMQTTClient::unsubscribe(const char *topic) {
  return _mqttClient.unsubscribe(topic);
}

void HydroServerMQTTClient::poll() { _mqttClient.poll(); }

bool HydroServerMQTTClient::isConnected() { return _mqttClient.connected(); }

String HydroServerMQTTClient::readMessage() {
  String message = "";
  while (_mqttClient.available()) {
    message += (char)_mqttClient.read();
  }
  return message;
}

String HydroServerMQTTClient::messageTopic() {
  return _mqttClient.messageTopic();
}