/*
  This file is part of HydroServerArduinoClient library.
  Sabin Panta
*/

#ifndef SRC_MODEMS_ESPRESSIFESP32_H_
#define SRC_MODEMS_ESPRESSIFESP32_H_

#define TINY_GSM_MODEM_ESP32
#define TINY_GSM_USE_WIFI true

#include "modem.h"
#include <TinyGsmClient.h>

#include <StreamDebugger.h>

class EspressifESP32 : public Modem {

public:
  EspressifESP32(Stream &xbeeSerial, int8_t powerPin, Stream &debugStream);
  EspressifESP32(Stream &xbeeSerial, int8_t powerPin);

  Client *createClient() override;

  Client *createSecureClient() override;

  bool connectToInternet(const char *ssid, const char *password,
                         uint32_t maxConnectionTime = 60000L) override;

  bool isInternetAvailable() override;

  // String getLocalIP() { return _modem.localIP().toString(); }

  uint32_t getNISTTime() override;

  void extraSetupForHTTPS(const char *host, const int port);

  void extraSetupForMQTT() override;

private:
  TinyGsm _modem;
  StreamDebugger _debugger;
};

#endif