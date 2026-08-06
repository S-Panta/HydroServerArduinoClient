/*
  This file is part of HydroServerArduinoClient library.
  Sabin Panta
*/

#ifndef SRC_HYDROSERVERHTTPCLIENT_H_
#define SRC_HYDROSERVERHTTPCLIENT_H_

#include "DataPublisher.h"
#include <Arduino.h>
#include <ArduinoHttpClient.h>

class HydroServerHTTPClient : public DataPublisher {
public:
  HydroServerHTTPClient(
      Client &client, const char *hydroServerURL = "playground.hydroserver.org",
      uint16_t port = 443, const char *apiKey = nullptr);

  void setApiKey(const char *apiKey);

  int getStatusCode() const;

  String getResponseBody() const;
  int publishObservation(const Observation &observation,
                         const char *phenomenonTime) override;

private:
  HttpClient _httpClient;
  const char *_apiKey;
  const char *_apiPath = "/api/sensorthings/v1.1/Observations";
  String _responseBody;
  int _statusCode;
};

#endif