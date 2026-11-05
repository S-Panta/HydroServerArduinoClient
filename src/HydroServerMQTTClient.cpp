/**
 * @file HydroServerMQTTClient.cpp
 * @author Sabin Panta
 * @brief Implementation of HydroServerMQTTClient.
 *
 * Part of the HydroServerArduinoClient library.
 */

#include "HydroServerMQTTClient.h"

// Main constructor. The client-only constructor below delegates to it.
HydroServerMQTTClient::HydroServerMQTTClient(Client &client, const char *broker,
                                             uint16_t port,
                                             const char *clientId,
                                             const char *username,
                                             const char *password)
    : _mqttClient(client), _port(port), _broker(broker),
      // Optional settings start empty and are filled in by the setters.
      _clientId(nullptr), _username(nullptr), _password(nullptr),
      _sitecode(nullptr) {

  setClientID(clientId);
  setAuthentication(username, password);
}

HydroServerMQTTClient::HydroServerMQTTClient(Client &client)
    : HydroServerMQTTClient(client, nullptr) {}

void HydroServerMQTTClient::setCleanSession(bool cleanSession) {
  // true: the broker forgets this client's state when it disconnects.
  // false: the broker keeps it, which only works with a fixed client ID
  // (a random ID is effectively always a clean session).
  // For QoS 0 publishing the difference doesn't matter.
  _cleanSession = cleanSession;
}

int HydroServerMQTTClient::connectToBroker() {
  // Uses whatever was set through the constructor, the setters, or the
  // long form of connectToBroker().

  if (_broker == nullptr || strlen(_broker) == 0) {
    return 0;
  }
  // ArduinoMqttClient takes these in milliseconds.
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
  // Without an ID, ArduinoMqttClient makes one up from millis(), so it changes
  // on every connection. A fixed, unique ID is what lets the broker resume
  // a persistent session.
  if (clientId != nullptr && strlen(clientId) > 0) {
    _clientId = clientId;
  }
}

void HydroServerMQTTClient::setSiteCode(const char *siteCode) {
  if (siteCode != nullptr && strlen(siteCode) > 0) {
    _sitecode = siteCode;
  }
}

void HydroServerMQTTClient::setAuthentication(const char *username,
                                              const char *password) {
  // Some brokers accept a user name with an empty password.
  if (username != nullptr && strlen(username) > 0 && password != nullptr) {
    _username = username;
    _password = password;
  }
}

const char *
HydroServerMQTTClient::getObservationTopic(const Observation &observation) {

  // siteCode/clientId/sensorId/observedProperty
  // The site code has to be set, or the topic is malformed and the broker
  // may close the connection. The other parts can technically be empty, but
  // the data is much harder to find that way.
  snprintf(_observationTopic, sizeof(_observationTopic), "%s/%s/%s/%s",
           _sitecode, _clientId, observation.sensorId,
           observation.observedProperty);

  return _observationTopic;
}

int HydroServerMQTTClient::publishObservation(const Observation &observation,
                                              const char *phenomenonTime) {
  // Any packet we send counts as keep-alive activity (MQTT-3.1.2-23). When
  // nothing is being published, poll() sends the PINGREQ instead.

  String payload = ObservationPayload::serialize(observation, phenomenonTime);

  if (!_mqttClient.connected()) {
    // connected() checks the TCP connection, so this catches a dropped
    // broker.
    return 0;
  };
  // TODO: support retain, QoS and dup through
  // beginMessage(topic, size, retain, qos, dup).
  // TODO: the default payload limit is 256 bytes; look into raising it.

  const char *topic = getObservationTopic(observation);
  _mqttClient.beginMessage(topic, (unsigned long)payload.length());
  _mqttClient.print(payload);
  // At QoS 0, endMessage() returns 1 even if the broker never gets the
  // message. Only QoS 1 and 2 report a failed send.
  return _mqttClient.endMessage();
}

int HydroServerMQTTClient::setLastWill(const char *payload, bool retain,
                                       int qos) {
  size_t payloadLength = strlen(payload);
  snprintf(_lastWillTopic, sizeof(_lastWillTopic), "%s/%s/lwt", _sitecode,
           _clientId);
  _mqttClient.beginWill(_lastWillTopic, payloadLength, retain, qos);
  _mqttClient.print(payload);
  return _mqttClient.endWill();
}

const char *HydroServerMQTTClient::getLastWillTopic() const {
  return _lastWillTopic;
}

int HydroServerMQTTClient::getConnectionError() {
  return _mqttClient.connectError();
}

void HydroServerMQTTClient::setKeepAliveInterval(unsigned long seconds) {
  _keepAliveSeconds = seconds;
}
void HydroServerMQTTClient::setConnectionTimeout(unsigned long timeout) {
  // How long to wait for the network connection to the broker. This isn't
  // part of the MQTT CONNECT packet itself.
  _connectionTimeout = timeout;
}

int HydroServerMQTTClient::subscribe(const char *topic) {
  if (!_mqttClient.connected())
    return 0;
  char fullTopic[256];
  snprintf(fullTopic, sizeof(fullTopic), "%s/%s", _sitecode, topic);
  // Subscribes at QoS 0.
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
    return _latestValue;
  }
}

// The callback only runs for topics this client has subscribed to.
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