/**
 * @file ESP32Bee.h
 * @author Sabin Panta
 * @brief Modem wrapper for the Espressif ESP32 Wi-Fi Bee on the Mayfly.
 *
 * Part of the HydroServerArduinoClient library
 */

#ifndef SRC_MODEMS_ESP32BEE_H_
#define SRC_MODEMS_ESP32BEE_H_

// TinyGSM picks its modem driver from these, so they have to come before
// TinyGsmClient.h.
#define TINY_GSM_MODEM_ESP32   ///< Tells TinyGSM to use its ESP32 driver.
#define TINY_GSM_USE_WIFI true ///< Tells TinyGSM the modem connects over Wi-Fi.

#include "modem.h"
#include <TinyGsmClient.h>

#include <StreamDebugger.h>

/**
 * @brief Helper class for  Espressif ESP32 Wi-Fi Bee on the Mayfly.
 *
 * Handles both plain TCP and TLS, so it works for MQTT, HTTP and HTTPS.
 * The ESP32 needs one extra step after joining Wi-Fi: call
 * extraSetupForMQTT() before MQTT, or extraSetupForHTTPS() before HTTPS.
 *
 * @code
 * ESP32Bee modem(Serial1, 18);
 * HydroServerMQTTClient mqttClient(*modem.createClient(), "broker.local");
 *
 * void setup() {
 *   Serial1.begin(57600);
 *   modem.connectToInternet(WIFI_SSID, WIFI_PASS);
 *   modem.extraSetupForMQTT();
 *   // ...then set up and connect mqttClient
 * }
 * @endcode
 *
 * @ingroup modems
 */
class ESP32Bee : public Modem {

public:
  /**
   * @brief Creates the modem and echoes all AT traffic to a debug stream.
   *
   * @param xbeeSerial Serial port the Bee is on: `Serial1` on the Mayfly.
   * @param powerPin Bee power pin: 18 on Mayfly v1.x.
   * @param debugStream Where to echo the AT commands, usually `Serial`.
   */
  ESP32Bee(Stream &xbeeSerial, int8_t powerPin, Stream &debugStream);

  /**
   * @brief Creates the modem.
   *
   * @param xbeeSerial Serial port the Bee is on: `Serial1` on the Mayfly.
   * @param powerPin Bee power pin: 18 on Mayfly v1.x.
   */
  ESP32Bee(Stream &xbeeSerial, int8_t powerPin);

  Client *createClient() override;

  /// @brief Creates a TLS client. Call extraSetupForHTTPS() before using it.
  /// @return The new client.
  Client *createSecureClient() override;

  bool connectToInternet(const char *ssid, const char *password,
                         uint32_t maxConnectionTime = 60000L) override;

  bool isInternetAvailable() override;

  /**
   * @brief Finds the baud rate the Bee is set to.
   *
   * Most ESP32 Bees run at 115200 or 57600, but not all of them. This tries
   * the common rates from 9600 up to 115200 and can take a while, so use it
   * when setting up a new Bee, not in a deployed sketch.
   *
   * @param serial Serial port the Bee is on.
   * @return The baud rate that worked, or 0 if none did.
   */
  uint32_t detectBaud(HardwareSerial &serial);

  /**
   * @copydoc Modem::getNISTTime()
   *
   * On the ESP32 this syncs with `pool.ntp.org` and waits up to 30 seconds
   * for an answer.
   */
  uint32_t getNISTTime() override;

  /**
   * @brief Prepares the ESP32 for HTTPS to one host.
   *
   * Some servers, the HydroServer playground among them, host several sites
   * on one IP address. They need the host name during the TLS handshake
   * (SNI) to pick the right certificate, and the ESP32 doesn't send it
   * unless told to. This sets the host name and opens the TLS connection.
   *
   * It also does what extraSetupForMQTT() does, so you don't need both.
   * Call it after connectToInternet().
   *
   * @param host Server host name, such as `playground.hydroserver.org`.
   * @param port Server port, usually 443.
   */
  void extraSetupForHTTPS(const char *host, const int port);

  /**
   * @brief Switches the ESP32 to the receive mode MQTT needs.
   *
   * Without this, the connection to the broker opens but then stalls. Call
   * it after connectToInternet() and before connecting to the broker.
   */
  void extraSetupForMQTT() override;

  /// @return The Bee's Wi-Fi MAC address in upper case, or an empty string
  /// if the modem didn't answer.
  String getMacAddress();

  /// @return Signal strength (RSSI) of the connected network, in dBm.
  int8_t getSignalQuality();

  /// @return The IP address the network gave the Bee.
  String getLocalIP() { return _modem.getLocalIP(); }

  /// @return The modem's identification text.
  String getModemInfo() { return _modem.getModemInfo(); }

  /// @return The Bee's AT firmware version, or an empty string if the modem
  /// didn't answer.
  String getFirmwareVersion();

private:
  TinyGsm _modem;
  StreamDebugger _debugger;
};

#endif
