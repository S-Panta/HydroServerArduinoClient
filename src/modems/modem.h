/**
 * @file modem.h
 * @author Sabin Panta
 * @brief Base class for the Mayfly Wi-Fi Bee modems.
 *
 * Part of the HydroServerArduinoClient library
 */
#ifndef SRC_MODEMS_MODEM_H_
#define SRC_MODEMS_MODEM_H_

#include <Arduino.h>
#include <Client.h>

/**
 * @brief What every Mayfly Wi-Fi Bee wrapper can do.
 *
 * Use ESP32Bee or DigiXbeeS6B. Both talk to
 * the Bee with AT commands through EnviroDIY's TinyGSM fork, and hand you an
 * Arduino `Client` you can pass to either HydroServer client.
 *
 * @ingroup modems
 */
class Modem {
public:
  /// @param powerPin Pin that switches power to the Bee socket. 18 on
  /// Mayfly v1.x.
  Modem(int8_t powerPin) : _powerPin(powerPin) {};

  virtual ~Modem() = default;

  /**
   * @brief Creates a plain TCP client that goes through the modem.
   *
   * Each call allocates a new client, so call it once, usually at global
   * scope, and pass the result to a HydroServer client.
   *
   * @return The new client.
   */
  virtual Client *createClient() = 0;

  /**
   * @brief Creates a TLS client for HTTPS, if the modem can do TLS.
   *
   * Like createClient(), each call allocates a new client.
   *
   * @return The new client, or `nullptr` if the modem has no TLS support.
   */
  virtual Client *createSecureClient() { return nullptr; }

  /// Switches on power to the Bee socket. connectToInternet() calls this
  /// for you.
  virtual void powerUp() {
    pinMode(_powerPin, OUTPUT);
    digitalWrite(_powerPin, HIGH);
  };

  /**
   * @brief Powers up the modem and joins a Wi-Fi network.
   *
   * @param ssid Network name.
   * @param password Network password.
   * @param maxConnectionTime How long to wait for the network to hand out
   * an address, in milliseconds. Default is 60 seconds.
   * @return `true` once the modem is on the network.
   */
  virtual bool connectToInternet(const char *ssid, const char *password,
                                 uint32_t maxConnectionTime = 60000L) = 0;

  /// @return `true` if the modem has an IP address and a DNS server.
  virtual bool isInternetAvailable() = 0;

  /**
   * @brief Gets the current UTC time from a network time server.
   *
   * The time is counted in seconds since 1 January 2000, which is what the
   * Mayfly's DS3231 clock uses, so you can pass it straight to
   * `rtc.setDateTime()`. Add 946684800 to get Unix time.
   *
   * Check the result before setting the clock: a failed lookup can return
   * 0. See each modem for details.
   *
   * @return Seconds since 2000-01-01 00:00:00 UTC.
   */
  virtual uint32_t getNISTTime() = 0;

  /**
   * @brief Does any modem setup MQTT needs.
   *
   * Call it after connectToInternet() and before connecting to the broker.
   */
  virtual void extraSetupForMQTT() = 0;

  // TODO: add getModemName() for debug output.
  // String getModemName();

private:
  // Pin that powers the Bee socket. 18 on Mayfly v1.x.
  int8_t _powerPin = 18;
};

#endif
