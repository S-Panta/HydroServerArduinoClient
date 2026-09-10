/*
  This file is part of HydroServerArduinoClient library.
  Sabin Panta
*/

#include "HydroServerHTTPClient.h"

HydroServerHTTPClient::HydroServerHTTPClient(Client &client,
                                             const char *hydroServerURL,
                                             uint16_t port, const char *apiKey)
    : _httpClient(client, hydroServerURL, port) {
  setApiKey(apiKey);
  _httpClient.connectionKeepAlive();
}

void HydroServerHTTPClient::setApiKey(const char *apiKey) {
  if (apiKey != nullptr) {
    _apiKey = apiKey;
  }
}

int HydroServerHTTPClient::_sendRequest(const char *method, const String &path,
                                        const String &requestBody) {
  _httpClient.beginRequest();

  if (strcmp(method, "GET") == 0) {
    _httpClient.get(path);
  } else if (strcmp(method, "POST") == 0) {
    _httpClient.post(path);
  } else {
    _httpClient.endRequest();
    return -1;
  }

  _httpClient.sendHeader("Accept", "*/*");
  _httpClient.sendHeader("X-Api-Key", _apiKey);
  _httpClient.sendHeader("Content-Type", "application/json");

  if (requestBody.length() > 0) {
    _httpClient.sendHeader("Content-Length", requestBody.length());
    _httpClient.beginBody();
    _httpClient.print(requestBody);
    _httpClient.endRequest();
  } else {
    _httpClient.endRequest();
  }

  int statusCode = _httpClient.responseStatusCode();
  _responseBody = _httpClient.responseBody();

  return statusCode;
}

bool HydroServerHTTPClient::isDatastreamAvailable(const char *datastreamId) {
  String path =
      "/api/sensorthings/v1.1/Datastreams('" + String(datastreamId) + "')";
  int statusCode = _sendRequest("GET", path, "");
  return statusCode == 200;
}

bool HydroServerHTTPClient::publishObservation(const Observation &observation,
                                               const char *phenomenonTime) {
  String requestBody =
      ObservationPayload::serialize(observation, phenomenonTime);
  String path = "/api/sensorthings/v1.1/Observations";
  int statusCode = _sendRequest("POST", path, requestBody);
  return statusCode == 200;
}

String HydroServerHTTPClient::getLastResponseBody() const {
  return _responseBody;
}
