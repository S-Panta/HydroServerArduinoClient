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
  // todo
  // secure client is not possible in xbee s6b
  return nullptr;
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

void ExpressifESP32::extraSetupForMQTT() {
  _modem.sendAT(GF("+CIPRECVMODE=1"));
  if (_modem.waitResponse() != 1) {
    return;
  }
}