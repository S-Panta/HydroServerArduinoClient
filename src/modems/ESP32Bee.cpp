/**
 * @file ESP32Bee.cpp
 * @author Sabin Panta
 * @brief Implementation of ESP32Bee.
 *
 * Part of the HydroServerArduinoClient library
 */
#include <modems/ESP32Bee.h>

ESP32Bee::ESP32Bee(Stream &xbeeSerial, int8_t powerPin)
    : Modem(powerPin), _modem(xbeeSerial) {}

ESP32Bee::ESP32Bee(Stream &xbeeSerial, int8_t powerPin, Stream &debugStream)
    : Modem(powerPin), _debugger(xbeeSerial, debugStream), _modem(_debugger) {}

Client *ESP32Bee::createClient() { return new TinyGsmClient(_modem); }

Client *ESP32Bee::createSecureClient() {
  return new TinyGsmClientSecure(_modem);
}

bool ESP32Bee::connectToInternet(const char *ssid, const char *password,
                                 uint32_t maxConnectionTime) {
  // Power up here in case the sketch didn't.
  powerUp();
  _modem.init();

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

bool ESP32Bee::isInternetAvailable() { return _modem.isNetworkConnected(); }

// Returns seconds since 2000-01-01 UTC, the DS3231's epoch.
uint32_t ESP32Bee::getNISTTime() {
  // Sync over UDP with pool.ntp.org, at a UTC offset of 0.
  _modem.NTPServerSync("pool.ntp.org", 0);
  // Wait up to 30 seconds (TinyGSM's default is 120).
  _modem.waitForTimeSync(30);
  uint32_t epoch = _modem.getNetworkEpoch(TinyGSM_EpochStart::UNIX);

  // The Mayfly's DS3231 counts seconds from 2000-01-01, not 1970-01-01, so
  // shift by the 946684800 seconds between the two.
  // https://www.envirodiy.org/ds3231-real-time-clock-rtc-date-conversion/
  uint32_t currentdateTime = epoch - 946684800UL;
  return currentdateTime;
}

void ESP32Bee::extraSetupForMQTT() {
  // Passive receive mode: incoming data waits in the ESP32's buffer until
  // TinyGSM asks for it. Has to be set before a TCP connection opens.
  // https://docs.espressif.com/projects/esp-at/en/release-v2.4.0.0/esp32/AT_Command_Set/TCP-IP_AT_Commands.html#at-ciprecvmode-query-set-socket-receiving-mode
  _modem.sendAT(GF("+CIPRECVMODE=1"));
}

void ESP32Bee::extraSetupForHTTPS(const char *host, const int port) {
  // Some HTTPS servers, like the HydroServer playground, host several domains
  // on one IP address. The modem has to send the host name during the TLS
  // handshake (SNI) so the server presents the right certificate. Without it
  // the connection can fail even though DNS and TCP both work.
  _modem.sendAT(GF("+CIPRECVMODE=1"));
  // Set SNI on both link IDs.
  // https://docs.espressif.com/projects/esp-at/en/release-v2.4.0.0/esp32/AT_Command_Set/TCP-IP_AT_Commands.html#at-cipsslcsni-query-set-ssl-client-server-name-indication-sni
  _modem.sendAT(GF("+CIPSSLCSNI=0,\""), host, GF("\""));
  _modem.sendAT(GF("+CIPSSLCSNI=1,\""), host, GF("\""));
  delay(3000);

  // Open the TLS connection, again on both link IDs.
  // https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Command_Set/TCP-IP_AT_Commands.html#cmd-start
  _modem.sendAT(GF("+CIPSTART=0,\"SSL\",\""), host, GF("\","), port);
  _modem.sendAT(GF("+CIPSTART=1,\"SSL\",\""), host, GF("\","), port);
}

// Can take several seconds.
uint32_t ESP32Bee::detectBaud(HardwareSerial &serial) {
  // TinyGSM tries each common rate from 9600 to 115200.
  return TinyGsmAutoBaud(serial);
}

String ESP32Bee::getMacAddress() {
  _modem.sendAT(GF("+CIPSTAMAC?"));
  // Reply looks like: +CIPSTAMAC:"24:6f:28:aa:bb:cc"
  if (_modem.waitResponse(2000L, GF("+CIPSTAMAC:\"")) != 1) {
    return "";
  }
  String mac = _modem.stream.readStringUntil('"');
  _modem.waitResponse(); // consume the trailing OK
  mac.toUpperCase();
  return mac;
}

int8_t ESP32Bee::getSignalQuality() {
  // Uses AT+CWJAP? and returns the RSSI of the connected AP
  return _modem.getSignalQuality();
}

String ESP32Bee::getFirmwareVersion() {
  _modem.sendAT(GF("+GMR"));
  if (_modem.waitResponse(2000L, GF("AT version:")) != 1) {
    return "";
  }
  String version = _modem.stream.readStringUntil('(');
  version.trim();

  _modem.waitResponse(1000L);
  while (_modem.stream.available()) {
    _modem.stream.read();
  }

  return version;
}