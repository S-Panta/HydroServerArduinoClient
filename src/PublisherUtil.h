/**
 * @file PublisherUtil.h
 * @author Sabin Panta
 * @brief The Observation struct and the JSON payload both clients send.
 *
 * Part of the HydroServerArduinoClient library, 
 */

/**
 * @defgroup observations Observations
 * @brief Describing a sensor reading and turning it into SensorThings JSON.
 */

/**
 * @defgroup clients Clients
 * @brief Sending observations to HydroServer over HTTP(S), or to an MQTT
 * broker.
 */

/**
 * @defgroup modems Mayfly modems
 * @brief Wrappers for the Wi-Fi Bees that plug into the EnviroDIY Mayfly.
 */

#ifndef SRC_PUBLISHERUTIL_H_
#define SRC_PUBLISHERUTIL_H_

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @brief One sensor reading, ready to send.
 *
 * The struct keeps pointers to the strings you give it, not copies, so use
 * string literals or strings that last for the whole program (globals or
 * `static`).
 *
 * @code
 * Observation temperature("temperature", "<datastream-uuid>", "temp-sensor-1");
 * temperature.value = 21.7;
 * @endcode
 *
 * @ingroup observations
 */
struct Observation {
  /// What is measured, such as `"temperature"`. Last part of the MQTT topic.
  const char *observedProperty;
  /// UUID of the HydroServer datastream this reading belongs to.
  const char *datastreamId;
  /// Your name for the physical sensor. Third part of the MQTT topic.
  const char *sensorId;
  /// The reading itself. `NAN` until you set it.
  double value;

  /**
   * @brief Creates an observation.
   *
   * @param observedProperty What is measured, such as `"temperature"`.
   * @param datastreamId HydroServer datastream UUID.
   * @param sensorId Your name for the physical sensor.
   * @param value The reading. Usually left out here and set before each
   * publish.
   */
  Observation(const char *observedProperty, const char *datastreamId,
              const char *sensorId, double value = NAN)
      : observedProperty(observedProperty), datastreamId(datastreamId),
        sensorId(sensorId), value(value) {}
};

/**
 * @brief Builds the SensorThings JSON that both clients send.
 * @ingroup observations
 */
namespace ObservationPayload {
/**
 * @brief Turns an observation into SensorThings JSON.
 *
 * The result looks like this:
 * @code{.json}
 * {
 *   "Datastream": { "@iot.id": "<datastreamId>" },
 *   "result": 21.7,
 *   "phenomenonTime": "2026-09-23T18:30:00Z"
 * }
 * @endcode
 *
 * You don't normally need to call this yourself. Both clients call it when
 * publishing.
 *
 * @param observation The reading to serialize.
 * @param phenomenonTime When the reading was taken, as an ISO 8601 UTC
 * string.
 * @return The JSON payload.
 */
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
