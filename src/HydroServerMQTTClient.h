/**
 * @file HydroServerMQTTClient.h
 * @author Sabin Panta
 * @brief MQTT client that publishes observations to a broker.
 *
 * Part of the HydroServerArduinoClient library
 */

#ifndef SRC_HYDROSERVERMQTTCLIENT_H_
#define SRC_HYDROSERVERMQTTCLIENT_H_

#include "PublisherUtil.h"
#include <Arduino.h>
#include <ArduinoMqttClient.h>

/**
 * @brief Publishes observations to an MQTT broker
 *
 * Observations are published to
 * `siteCode/clientId/sensorId/observedProperty`, so call setSiteCode() and
 * setClientID() before connecting. The payload is the same SensorThings JSON
 * that HydroServerHTTPClient sends.
 *
 * The broker only relays messages. To get them into HydroServer you need a
 * separate service that subscribes to the broker and writes the observations
 * to HydroServer.
 *
 * @code
 * WiFiClient wifiClient;
 * HydroServerMQTTClient mqttClient(wifiClient, "test.mosquitto.org");
 *
 * void setup() {
 *   // ...join Wi-Fi and set the clock first
 *   mqttClient.setSiteCode("mysite");
 *   mqttClient.setClientID("logger01");
 *   mqttClient.setLastWill("offline");
 *   mqttClient.connectToBroker();
 * }
 *
 * void loop() {
 *   mqttClient.poll();
 *   // ...when it's time for a reading:
 *   mqttClient.publishObservation(temperature, getISO8601Time());
 * }
 * @endcode
 *
 * @ingroup clients
 */
class HydroServerMQTTClient {
public:
  static constexpr uint16_t defaultPort = 1883;

  /**
   * @brief Creates a client for a known broker.
   *
   * Only the network client and broker are required. You can leave the rest
   * out here and set them later with setClientID() and setAuthentication().
   *
   * @param client Network client to use, such as `WiFiClient` or
   * `*modem.createClient()`.
   * @param broker Broker host name or IP address.
   * @param port Broker port. Defaults to 1883.
   * @param clientId Name the broker knows this logger by. Also the second
   * part of every topic.
   * @param username Broker user name, if it needs one.
   * @param password Broker password. Can be an empty string for brokers that
   * accept a user name on its own.
   */
  HydroServerMQTTClient(Client &client, const char *broker,
                        uint16_t port = defaultPort,
                        const char *clientId = nullptr,
                        const char *username = nullptr,
                        const char *password = nullptr);

  /**
   * @brief Creates a client before the broker is known.
   *
   * Give the broker later through the long form of connectToBroker().
   *
   * @param client Network client to use.
   */
  HydroServerMQTTClient(Client &client);

  /**
   * @brief Sets the site code, the first part of every topic.
   *
   * Use your HydroServer site's code (see
   * [managing site metadata](https://hydroserver.org/user-guides/how-to/managing-site-metadata.html#managing-site-metadata)).
   * Call this before setLastWill() and connectToBroker(), since both use it.
   *
   * @param siteCode The site code. Empty or null values are ignored.
   */
  void setSiteCode(const char *siteCode);

  /**
   * @brief Sets the broker user name and password.
   *
   * Only needed if you didn't pass them to the constructor. Call it before
   * connecting.
   *
   * @param username User name. If it's empty or null, nothing changes.
   * @param password Password. May be empty, but not null.
   */
  void setAuthentication(const char *username, const char *password);

  /**
   * @brief Sets the client ID, the second part of every topic.
   *
   * Give each logger its own ID. The broker uses it to recognise a logger
   * when it reconnects, and two loggers with the same ID keep pushing each
   * other off the broker.
   *
   * @param clientId The client ID. Empty or null values are ignored.
   */
  void setClientID(const char *clientId);

  /**
   * @brief Sets the message the broker sends out if this logger drops off
   * without disconnecting cleanly.
   *
   * The message goes to `siteCode/clientId/lwt`, so call setSiteCode() and
   * setClientID() first, and call this before connecting. QoS 1 or higher is
   * recommended so the broker doesn't lose the message.
   *
   * @param payload The message, such as `"offline"`.
   * @param retain If `true`, the broker keeps the message for anyone who
   * subscribes later.
   * @param qos Quality of service: 0, 1 or 2.
   * @return 1 on success, 0 on failure.
   */
  int setLastWill(const char *payload, bool retain = true, int qos = 1);

  /// @return The topic setLastWill() publishes to, so you can check it.
  const char *getLastWillTopic() const;

  /**
   * @brief Chooses whether the broker forgets this client between
   * connections.
   *
   * With `true`, the broker drops this client's subscriptions and queued
   * messages every time it reconnects. With `false` (the default) it keeps
   * them, which only works if the client ID stays the same. For QoS 0
   * messages it makes no difference.
   *
   * @param cleanSession `true` to start fresh on every connection.
   */
  void setCleanSession(bool cleanSession);

  /**
   * @brief Sets how often, in seconds, the client pings the broker.
   *
   * The broker drops a client it hasn't heard from in about 1.5 times this
   * interval. The default is 60 seconds. If your logger sleeps between
   * readings, either make this longer than the sleep or reconnect after
   * waking. Call it before connecting.
   *
   * @param seconds Keep-alive interval in seconds.
   */
  void setKeepAliveInterval(unsigned long seconds);

  /**
   * @brief Sets how long, in seconds, to wait for the broker when
   * connecting.
   *
   * The default is 20 seconds.
   *
   * @param seconds Timeout in seconds.
   */
  void setConnectionTimeout(unsigned long seconds);

  /**
   * @brief Connects to a broker given here rather than in the constructor.
   *
   * The parameters mean the same as in the constructor. If you leave out
   * the client ID, user name or password, the ones already set are kept.
   *
   * @return 1 if connected, 0 if not. getConnectionError() says why.
   */
  int connectToBroker(const char *broker, uint16_t port = defaultPort,
                      const char *clientId = nullptr,
                      const char *username = nullptr,
                      const char *password = nullptr);

  /**
   * @brief Connects using the broker and settings you've already given.
   *
   * @return 1 if connected, 0 if not. Also returns 0 straight away if no
   * broker was set. Otherwise getConnectionError() says why.
   */
  int connectToBroker();

  /// @return `true` while the connection to the broker is open.
  bool isConnected();

  /**
   * @brief Returns the error code from the last failed connection attempt.
   *
   * The codes come from ArduinoMqttClient. Negative numbers are network
   * problems: `-2` usually means the broker host or port can't be reached.
   * Positive numbers mean the broker refused the connection, for example
   * because of a wrong user name or password.
   *
   * @return The error code, or 0 if the last attempt succeeded.
   */
  int getConnectionError();

  /**
   * @brief Starts listening on a topic under this site.
   *
   * The site code is added in front, so `subscribe("logger01/pump")` listens
   * on `siteCode/logger01/pump`. Set a handler with onMessage() to receive
   * the messages.
   *
   * @param topic Topic without the site code.
   * @return 1 on success, 0 if not connected or the broker refused.
   */
  int subscribe(const char *topic);

  /**
   * @brief Reads the incoming message as SensorThings JSON and saves its
   * `result`.
   *
   * Call this from your onMessage() handler, then read the value with
   * getLatestValueofTopic(). This consumes the message, so don't also call
   * readMessage() for the same one.
   */
  void deserializePayload();

  /**
   * @brief Returns the value deserializePayload() saved for a topic.
   *
   * The site code is added in front of @p topic, the same way as in
   * subscribe().
   *
   * @param topic Topic without the site code.
   * @return The saved value. Only meaningful if the latest message arrived
   * on this topic.
   */
  float getLatestValueofTopic(const char *topic);

  /**
   * @brief Stops listening on a topic.
   *
   * Unlike subscribe(), this doesn't add the site code, so pass the full
   * topic.
   *
   * @param topic Full topic, including the site code.
   * @return 1 on success, 0 on failure.
   */
  int unsubscribe(const char *topic);

  /**
   * @brief Keeps the connection alive and delivers incoming messages.
   *
   * Call this on every pass through `loop()`. It sends the keep-alive pings
   * and runs your onMessage() handler when a message arrives. If it isn't
   * called often enough, the broker decides the logger is gone and drops
   * the connection. See setKeepAliveInterval().
   */
  void poll();

  /**
   * @brief Sets a function to run when a message arrives on a subscribed
   * topic.
   *
   * The function gets the message size in bytes. Inside it, messageTopic()
   * tells you where the message came from, and readMessage() or
   * deserializePayload() reads it.
   *
   * @code
   * void handleMessage(int size) {
   *   Serial.print(mqttClient.messageTopic());
   *   Serial.print(": ");
   *   Serial.println(mqttClient.readMessage());
   * }
   *
   * mqttClient.onMessage(handleMessage);
   * @endcode
   *
   * @param callback Function to call for each message.
   */
  void onMessage(void (*callback)(int));

  /// @brief Reads the incoming message.
  /// @return The message as text.
  String readMessage();

  /// @return The topic the incoming message arrived on.
  String messageTopic();

  /**
   * @brief Publishes one observation.
   *
   * The topic is built from the site code, the client ID, and the
   * observation's sensor ID and observed property.
   *
   * @param observation The reading to send.
   * @param phenomenonTime When the reading was taken, as an ISO 8601 UTC
   * string such as `2026-09-23T18:30:00Z`.
   * @return 1 on success, 0 if not connected or sending failed. Messages go
   * out at QoS 0, where 1 only means the message left the logger, not that
   * the broker got it.
   */
  int publishObservation(const Observation &observation,
                         const char *phenomenonTime);

  /**
   * @brief Returns the topic an observation would be published to.
   *
   * Handy when data isn't showing up where you expect: print this to
   * Serial and compare. The text is overwritten by the next call.
   *
   * @param observation The observation to build the topic for.
   * @return The full topic.
   */
  const char *getObservationTopic(const Observation &observation);

  /**
   * @brief Publishes several observations with the same timestamp.
   *
   * @param observations Array of pointers to the observations to send.
   * @param size Number of entries in @p observations.
   * @param phenomenonTime Timestamp used for all of them.
   * @return How many were published successfully.
   */
  uint8_t publishAll(Observation **observations, uint8_t size,
                     const char *phenomenonTime) {
    uint8_t published = 0;
    for (uint8_t i = 0; i < size; i++) {
      if (publishObservation(*observations[i], phenomenonTime)) {
        published++;
      }
    }
    return published;
  }

private:
  MqttClient _mqttClient;
  const char *_broker;
  uint16_t _port;
  const char *_clientId = "logger-1";
  const char *_username;
  const char *_password;
  unsigned long _keepAliveSeconds = 60;
  unsigned long _connectionTimeout = 20;
  bool _cleanSession = false;
  const char *_sitecode = "site1";
  float _latestValue;
  char _observationTopic[256];
  char _lastWillTopic[128];
};

#endif
