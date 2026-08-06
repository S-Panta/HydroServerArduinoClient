/*
  This file is part of HydroServerArduinoClient library.
  Sabin Panta
*/

#ifndef SRC_PUBLISHERUTIL_H_
#define SRC_PUBLISHERUTIL_H_

#include <Arduino.h>
#include <ArduinoJson.h>

struct Observation {
  const char *observedProperty;
  const char *datastreamId;
  const char *sensorId;
  double value = 0.0;
};

namespace ObservationPayload {
inline String serialize(const Observation &observation,
                        const char *phenomenonTime) {
  JsonDocument doc;
  doc["Datastream"]["@iot.id"] = observation.datastreamId;
  doc["result"] = observation.value;
  doc["phenomenonTime"] = phenomenonTime;
  String payload;
  serializeJson(doc, payload);
  return payload;
}
}; // namespace ObservationPayload
#endif