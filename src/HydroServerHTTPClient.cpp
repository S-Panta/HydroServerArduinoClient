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

int HydroServerHTTPClient::publishObservation(const Observation &observation,
                                              const char *phenomenonTime) {
  String requestBody = createObservationPayload(observation, phenomenonTime);
  _httpClient.beginRequest();
  _httpClient.post(_apiPath);
  _httpClient.sendHeader("accept", "*/*");
  _httpClient.sendHeader("X-Api-Key", _apiKey);
  _httpClient.sendHeader("Content-Type", "application/json");
  _httpClient.sendHeader("Content-Length", requestBody.length());
  _httpClient.beginBody();
  _httpClient.print(requestBody);
  _httpClient.endRequest();

  _statusCode = _httpClient.responseStatusCode();
  _responseBody = _httpClient.responseBody();
  return _statusCode == 201;
}

// read-only method has const after a function declaration
// const objects are only allowed to call these const functions
int HydroServerHTTPClient::getStatusCode()const {
  return _statusCode;
}

String HydroServerHTTPClient::getResponseBody() const { return _responseBody; }
