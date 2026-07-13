#ifndef SRC_DATAPUBLISHER_H_
#define SRC_DATAPUBLISHER_H_

#include <Arduino.h>
#include <ArduinoJson.h>

struct Observation {
  const char *observedProperty;
  const char *datastreamId;
  double value = 0.0;
};

class DataPublisher {
protected:
  static String createObservationPayload(const Observation &observation,
                                         const char *phenomenonTime) {
    JsonDocument doc;
    doc["Datastream"]["@iot.id"] = observation.datastreamId;
    doc["result"] = observation.value;
    doc["phenomenonTime"] = phenomenonTime;
    String payload;
    serializeJson(doc, payload);
    return payload;
  }

public:
  // both http and mqtt publisher must implement this
  virtual int publishObservation(const Observation &observation,
                                 const char *phenomenonTime);

  virtual void publishAll(Observation **observations, uint8_t count,
                          const char *phenomenonTime) {
    for (uint8_t i = 0; i < count; i++) {
      publishObservation(*observations[i], phenomenonTime);
    }
  }

  virtual ~DataPublisher() = default;
};

#endif