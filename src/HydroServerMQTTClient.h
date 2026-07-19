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
  // default argument to constructor are passed here
  // nullptr so that these arguments can be ommitted in constructor
  HydroServerMQTTClient(Client &client, const char *broker,
                        uint16_t port = defaultPort,
                        const char *clientId = nullptr,
                        const char *username = nullptr,
                        const char *password = nullptr);

  HydroServerMQTTClient(Client &client);

  // Sets the site code used as the first segment of the MQTT topic path
  // for published observations and the last will message
  // e.g "<siteCode>/<clientId>/<sensorId>/<observedProperty>/observations".
  // Must be called before connectToBroker() / publishObservation()
  // See site metadata detail
  // :https://hydroserver.org/user-guides/how-to/managing-site-metadata.html#managing-site-metadata
  void setSiteCode(const char *siteCode);

  // if the class is initalized without constructor
  // these functions will set mqtt configs
  void setAuthentication(const char *username, const char *password);
  void setClientID(const char *clientId);

  int setLastWill(const char *payload);

  // session related function
  void setCleanSession(bool cleanSession);
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
  int unsubscribe(const char *topic);
  void poll();

  void onMessage(void (*callback)(int));
  String readMessage();
  String messageTopic();

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
};

#endif