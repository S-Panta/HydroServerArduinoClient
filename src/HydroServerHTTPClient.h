/**
 * @file HydroServerHTTPClient.h
 * @author Sabin Panta
 * @brief HTTP(S) client that posts observations to HydroServer's
 * SensorThings API.
 *
 * Part of the HydroServerArduinoClient library
 */

#ifndef SRC_HYDROSERVERHTTPCLIENT_H_
#define SRC_HYDROSERVERHTTPCLIENT_H_

#include "PublisherUtil.h"
#include <Arduino.h>
#include <ArduinoHttpClient.h>

/**
 * @brief Posts observations straight to a HydroServer instance.
 *
 * Each call to publishObservation() sends one `POST` to
 * `/api/sensorthings/v1.1/Observations`, with your API key in the
 * `X-Api-Key` header. The connection is kept open between requests, so
 * posting several readings in a row doesn't reconnect each time.
 *
 * @code
 * WiFiSSLClient sslClient;
 * HydroServerHTTPClient hsClient(sslClient, "playground.hydroserver.org", 443,
 *                                API_KEY);
 *
 * if (!hsClient.publishObservation(temperature, "2026-09-23T18:30:00Z")) {
 *   Serial.println(hsClient.getLastResponseBody());
 * }
 * @endcode
 *
 * @ingroup clients
 */
class HydroServerHTTPClient {
public:
  /**
   * @brief Creates a client for a HydroServer instance.
   *
   * @param client Network client the requests go through. Port 443 needs a
   * TLS client, such as `WiFiSSLClient` on the Uno R4 WiFi or
   * `*modem.createSecureClient()` with the Mayfly's ESP32 Bee.
   * @param hydroServerURL Host name only, without `https://` or a path.
   * @param port 443 for HTTPS, or 80 for a server on plain HTTP.
   * @param apiKey HydroServer API key with write access to your datastreams.
   * You can set it later with setApiKey() instead, as long as it's set
   * before the first request.
   */
  HydroServerHTTPClient(
      Client &client, const char *hydroServerURL = "playground.hydroserver.org",
      uint16_t port = 443, const char *apiKey = nullptr);

  /**
   * @brief Sets the API key sent with every request.
   *
   * @param apiKey The key.
   */
  void setApiKey(const char *apiKey);

  /**
   * @brief Returns the body of HydroServer's last response.
   *
   * Print this when a request fails. HydroServer's error message usually
   * says what went wrong, such as an unknown datastream or a key without
   * access.
   *
   * @return The response body, or an empty string before the first request.
   */
  String getLastResponseBody() const;

  /**
   * @brief Checks that a datastream exists and your API key can see it.
   *
   * Worth calling once in `setup()`: it catches a mistyped UUID or a key
   * without access before the logger goes out to the field.
   *
   * @param datastreamid HydroServer datastream UUID.
   * @return `true` if HydroServer answered with status 200. Otherwise
   * `false`, and getLastResponseBody() has the reason.
   */
  bool isDatastreamAvailable(const char *datastreamid);

  /**
   * @brief Posts one observation to HydroServer.
   *
   * @param observation The reading to send. Its `datastreamId` decides which
   * datastream it lands in.
   * @param phenomenonTime When the reading was taken, as an ISO 8601 UTC
   * string such as `2026-09-23T18:30:00Z`.
   * @return `true` if HydroServer answered with status 200. Otherwise
   * `false`, and getLastResponseBody() has the reason.
   */
  bool publishObservation(const Observation &observation,
                          const char *phenomenonTime);

  /**
   * @brief Posts several observations with the same timestamp.
   *
   * Sends one request per observation. A failed post doesn't stop the rest,
   * but this doesn't tell you which ones failed. If you need to know, call
   * publishObservation() in a loop instead.
   *
   * @param observations Array of pointers to the observations to send.
   * @param size Number of entries in @p observations.
   * @param phenomenonTime Timestamp used for all of them.
   */
  void publishAll(Observation **observations, uint8_t size,
                  const char *phenomenonTime) {
    for (uint8_t i = 0; i < size; i++) {
      publishObservation(*observations[i], phenomenonTime);
    }
  }

private:
  HttpClient _httpClient;
  const char *_apiKey;
  String _responseBody;
  // Sends one GET or POST, keeps the response body, and returns the HTTP
  // status code (-1 for any other method).
  int _sendRequest(const char *method, const String &path, const String &body);
};

#endif
