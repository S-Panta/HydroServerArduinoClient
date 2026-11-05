/**
 * @file DigiXbeeS6B.cpp
 * @author Sabin Panta
 * @brief Implementation of DigiXbeeS6B.
 *
 * Part of the HydroServerArduinoClient library
 */
#include <modems/DigiXbeeS6B.h>

DigiXbeeS6B::DigiXbeeS6B(Stream &xbeeSerial, int8_t powerPin)
    : Modem(powerPin), _modem(xbeeSerial) {}

DigiXbeeS6B::DigiXbeeS6B(Stream &xbeeSerial, int8_t powerPin,
                         Stream &debugStream)
    : Modem(powerPin), _debugger(xbeeSerial, debugStream), _modem(_debugger) {}

Client *DigiXbeeS6B::createClient() { return new TinyGsmClient(_modem); }

Client *DigiXbeeS6B::createSecureClient() {
  // The S6B has no TLS support.
  return nullptr;
}

bool DigiXbeeS6B::connectToInternet(const char *ssid, const char *password,
                                    uint32_t maxConnectionTime) {
  // Power up here in case the sketch didn't.
  powerUp();
  if (!_modem.networkConnect(ssid, password)) {
    return false;
  }

  if (!(isInternetAvailable())) {
    if (!_modem.waitForNetwork(maxConnectionTime)) {
      // TODO: print a debug message here.
      return false;
      // PRINTOUT(F("... WiFi connection failed"));
    }
  };
  return true;
}

bool DigiXbeeS6B::isInternetAvailable() { return _modem.isNetworkConnected(); }

// Returns seconds since 2000-01-01 UTC, the DS3231's epoch.
uint32_t DigiXbeeS6B::getNISTTime() {
  if (!isInternetAvailable()) {
    return 0;
  }
  // The XBee can't have UDP (NTP) and TCP open at the same time, and MQTT
  // and HTTP need TCP, so ask NIST's TCP time service instead. This is one
  // of the time-[a,b,c,d]-wwv.nist.gov servers.
  const char *nistIP = "132.163.97.1";

  Client *client = createClient();
  bool connected = client->connect(nistIP, NIST_PORT);
  if (connected) {
    // The server answers once it receives something.
    client->println('!');
    uint32_t startTime = millis();
    while (client && client->available() < NIST_RESPONSE_BYTES &&
           millis() - startTime < 5000L) {
      // Wait up to 5 seconds for the 4-byte answer.
    }

    if (client->available() >= NIST_RESPONSE_BYTES) {
      uint8_t buffer[NIST_RESPONSE_BYTES];
      client->read(buffer, NIST_RESPONSE_BYTES);
      client->stop();
      uint32_t ntpTime = ((uint32_t)buffer[0] << 24) |
                         ((uint32_t)buffer[1] << 16) |
                         ((uint32_t)buffer[2] << 8) | (uint32_t)buffer[3];
      // NTP time counts seconds since 1900-01-01 00:00:00 UTC.
      // Unix time counts seconds since 1970-01-01 00:00:00 UTC.
      // The difference between those two epochs is 2208988800 seconds.
      uint32_t unixTime = ntpTime - 2208988800UL;

      // The Mayfly's DS3231 counts seconds from 2000-01-01, not 1970-01-01,
      // so shift by the 946684800 seconds between the two.
      // https://www.envirodiy.org/ds3231-real-time-clock-rtc-date-conversion/
      uint32_t currentdateTime = unixTime - 946684800UL;
      return currentdateTime;
    } else {
      client->stop();
    }
  } else {
    return 0;
  }
}

void DigiXbeeS6B::extraSetupForMQTT() {
  // Nothing needed yet. If it turns out to be, this is where ATDL and the
  // port would be set before opening the TCP connection.
}