/*
  This file is part of HydroServerArduinoClient library.
  Sabin Panta
*/
#include <modems/ExpressifESP32.h>

ExpressifESP32::ExpressifESP32(Stream &xbeeSerial, int8_t powerPin)
    : Modem(powerPin), _modem(xbeeSerial) {}

ExpressifESP32::ExpressifESP32(Stream &xbeeSerial, int8_t powerPin,
                               Stream &debugStream)
    : Modem(powerPin), _debugger(xbeeSerial, debugStream), _modem(_debugger) {}

Client *ExpressifESP32::createClient() { return new TinyGsmClient(_modem); }

Client *ExpressifESP32::createSecureClient() {
  return new TinyGsmClientSecure(_modem);
}

bool ExpressifESP32::connectToInternet(const char *ssid, const char *password,
                                       uint32_t maxConnectionTime) {
  // make sure the modem was powered on first
  powerUp();
  // This does some configuration in the Esp32 modem
  _modem.init();

  if (!_modem.networkConnect(ssid, password)) {
    return false;
  }

  // this checks whether local IP and DNS have been allocated
  // and not 0.0.0.0
  if (!(isInternetAvailable())) {
    // waits and check for isNetworkConnected()
    // default time is 60s
    if (!_modem.waitForNetwork(maxConnectionTime)) {
      // debug the output as network not connected
      return false;
      // PRINTOUT(F("... WiFi connection failed"));
    }
  };
  return true;
}

bool ExpressifESP32::isInternetAvailable() {
  return _modem.isNetworkConnected();
}

// returns time in utc
uint32_t ExpressifESP32::getNISTTime() {
  // opens UDP socket for time server
  // by default, the time server is pool.ntp.org.
  // by default, it returns UTC time
  _modem.NTPServerSync("pool.ntp.org", 0);
  // this function waits for 120 seconds by defaults
  _modem.waitForTimeSync(30);
  uint32_t epoch = _modem.getNetworkEpoch(TinyGSM_EpochStart::UNIX);

  // Mayfly logger has Onboard realtime clock (RTC) (DS3231)
  // DS3231 chip give the number of seconds since January 1, 2000
  // Unix Time, which is the number of seconds since 1/1/1970
  // https://www.envirodiy.org/ds3231-real-time-clock-rtc-date-conversion/
  // DateTime dt(unixTime - 946684800UL);
  uint32_t currentdateTime = epoch - 946684800UL;
  return currentdateTime;
}

void ExpressifESP32::extraSetupForMQTT(){
  // See :https://docs.espressif.com/projects/esp-at/en/release-v2.4.0.0/esp32/AT_Command_Set/TCP-IP_AT_Commands.html#at-ciprecvmode-query-set-socket-receiving-mode
  // This should be done before opening tcp connection
  _modem.sendAT(GF("+CIPRECVMODE=1"));
}


void ExpressifESP32::extraSetupForHTTPS(const char* host, const int port) {
  // Some HTTPS servers(like playground instance of hydroserver) host multiple domains on the same IP address.
  // During the TLS handshake, the modem must send the hostname using
  // Server Name Indication so the server can present the correct
  // SSL/TLS certificate. Without SNI, the connection may fail even
  // though DNS resolution and TCP connectivity succeed.
  _modem.sendAT(GF("+CIPRECVMODE=1"));
  // Ref: https://docs.espressif.com/projects/esp-at/en/release-v2.4.0.0/esp32/AT_Command_Set/TCP-IP_AT_Commands.html#at-cipsslcsni-query-set-ssl-client-server-name-indication-sni
  _modem.sendAT(GF("+CIPSSLCSNI=0,\""), host, GF("\""));
  _modem.sendAT(GF("+CIPSSLCSNI=1,\""), host, GF("\""));
  delay(3000);

  // See : https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Command_Set/TCP-IP_AT_Commands.html#cmd-start
  // same as above. This also need to be done twice
  _modem.sendAT(GF("+CIPSTART=0,\"SSL\",\""), host, GF("\","), port);
  _modem.sendAT(GF("+CIPSTART=1,\"SSL\",\""), host, GF("\","), port);
}