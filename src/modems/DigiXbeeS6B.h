/**
 * @file DigiXbeeS6B.h
 * @author Sabin Panta
 * @brief Modem wrapper for the Digi XBee S6B Wi-Fi Bee on the Mayfly.
 *
 * Part of the HydroServerArduinoClient library
 */

#ifndef SRC_MODEMS_DIGIXBEES6B_H_
#define SRC_MODEMS_DIGIXBEES6B_H_

#define TINY_GSM_MODEM_XBEE
#define TINY_GSM_USE_WIFI true

/// Port of NIST's time service (RFC 868). The XBee uses it over TCP because
/// it can't use NTP while a TCP connection is open.
#define NIST_PORT 37

/// NIST's time service answers with 4 bytes.
#define NIST_RESPONSE_BYTES 4

#include "modem.h"
#include <StreamDebugger.h>
#include <TinyGsmClient.h>

/**
 * @brief Helper for Digi XBee S6B Wi-Fi Bee on the Mayfly.
 *
 * The S6B only does plain TCP, so use it for MQTT or plain HTTP. For HTTPS,
 * use ESP32Bee.
 *
 * @code
 * DigiXbeeS6B modem(Serial1, 18);
 * HydroServerMQTTClient mqttClient(*modem.createClient(), "broker.local");
 *
 * void setup() {
 *   Serial1.begin(9600);
 *   modem.connectToInternet(WIFI_SSID, WIFI_PASS);
 *   // ...then set up and connect mqttClient
 * }
 * @endcode
 *
 * @ingroup modems
 */
class DigiXbeeS6B : public Modem {
public:
  /**
   * @brief Creates the modem and echoes all AT traffic to a debug stream.
   *
   * @param xbeeSerial Serial port the Bee is on: `Serial1` on the Mayfly.
   * @param powerPin Bee power pin: 18 on Mayfly v1.x.
   * @param debugStream Where to echo the AT commands, usually `Serial`.
   */
  DigiXbeeS6B(Stream &xbeeSerial, int8_t powerPin, Stream &debugStream);

  /**
   * @brief Creates the modem.
   *
   * @param xbeeSerial Serial port the Bee is on: `Serial1` on the Mayfly.
   * @param powerPin Bee power pin: 18 on Mayfly v1.x.
   */
  DigiXbeeS6B(Stream &xbeeSerial, int8_t powerPin);

  Client *createClient() override;

  /// @brief The S6B can't do TLS.
  /// @return Always `nullptr`.
  Client *createSecureClient() override;

  bool connectToInternet(const char *ssid, const char *password,
                         uint32_t maxConnectionTime = 60000L) override;

  // String getLocalIP() { return _modem.localIP().toString(); }

  bool isInternetAvailable() override;

  /**
   * @copydoc Modem::getNISTTime()
   *
   * The XBee can't use NTP while a TCP connection is open, so this asks
   * NIST's time service over TCP instead. It returns 0 if the modem isn't
   * on the network or the server doesn't connect.
   */
  uint32_t getNISTTime() override;

  /// Does nothing on the S6B for now.
  void extraSetupForMQTT() override;

private:
  TinyGsm _modem;
  StreamDebugger _debugger;
};
#endif
