/*
  This file is part of HydroServerArduinoClient library.
  Sabin Panta
*/

#ifndef SRC_HYDROSERVERMQTTCLIENT_H_
#define SRC_HYDROSERVERMQTTCLIENT_H_

#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <DataPublisher.h>

class HydroServerMQTTClient : public DataPublisher {
public:
  static constexpr uint16_t defaultPort = 1883;

  // Optional args default to nullptr so that these arguments can be ommitted and set later by setters.
  HydroServerMQTTClient(Client &client, const char *broker,
                        uint16_t port = defaultPort,
                        const char *clientId = nullptr,
                        const char *username = nullptr,
                        const char *password = nullptr);
  
  // Constructor for when only the network client is known at init time;
  // broker, port, clientId, etc. must be set later via setters.
  HydroServerMQTTClient(Client &client);

  // Sets the site code used as the first segment of the MQTT topic path
  // for published observations and the last will message.
  // e.g "<siteCode>/<clientId>/<sensorId>/<observedProperty>/observations".
  // See site metadata detail
  // :https://hydroserver.org/user-guides/how-to/managing-site-metadata.html#managing-site-metadata
  // This must be called before connectToBroker() / publishObservation()
  void setSiteCode(const char *siteCode);

  // if the class is initalized without constructor
  // these functions will set mqtt configs
  void setAuthentication(const char *username, const char *password);
  void setClientID(const char *clientId);

  // If the client disconnects unexpectly,this will define the final message
  // the message will be published in sitecode/device_id/lwt topic
  int setLastWill(const char *payload);

  // cleanSession=true: broker discards prior subscriptions/queued messages for this client on reconnect.
  // use setClientID before setting this to true
  // Broker should know the client before it accept command for persistence session
  void setCleanSession(bool cleanSession);

  // Sets how often (in seconds) a keep-alive ping is sent to the broker
  // to indicate the client is still connected.
  // default is 60 seconds
  void setKeepAliveInterval(unsigned long seconds);

  void setConnectionTimeout(unsigned long seconds);

  int connectToBroker(const char *broker, uint16_t port = defaultPort,
                      const char *clientId = nullptr,
                      const char *username = nullptr,
                      const char *password = nullptr);

  // This function can be used when all mqtt config are passed in constructor
  int connectToBroker();

  bool isConnected();
  int getConnectionError();

  int subscribe(const char *topic);

  // Parses the most recently received raw MQTT payload into
  // readable topic/value pairs, used by getLatestValueofTopic().
  void deserializePayload();

  // Returns the most recent value received for the given topic.
  float getLatestValueofTopic(const char *topic);

  int unsubscribe(const char *topic);

  // Must be called regularly (e.g. in loop()) to process incoming
  // messages and maintain the broker connection
  void poll();
  
// Registers a callback to handle incoming MQTT messages.
//
// The callback is executed once for each incoming message received on a subscribed topic
// Example:
//   void doSomethingToIncomingMessage(int message) {};
//   mqttClient.onMessage(doSomethingToIncomingMessage);
  void onMessage(void (*callback)(int));

// Reads and returns the payload of the most recently received message
  String readMessage();

  // Returns the topic the most recently received message was published to.
  String messageTopic();

  // publish Observation in json format
  int publishObservation(const Observation &observation,
                         const char *phenomenonTime) override;

private:
  MqttClient _mqttClient;
  // Private class variable are assigned value later
  const char *_broker;
  uint16_t _port;
  const char *_clientId;
  const char *_username;
  const char *_password;
  unsigned long _keepAliveSeconds = 60;
  unsigned long _connectionTimeout = 20;
  bool _cleanSession = false;
  const char *_sitecode;
  float _latestValue;
};

#endif