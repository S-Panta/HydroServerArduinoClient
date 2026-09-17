/*
  This file is part of HydroServerArduinoClient library.
  Sabin Panta
*/

#ifndef SRC_MODEMS_DIGIXBEES6B_H_
#define SRC_MODEMS_DIGIXBEES6B_H_

#define TINY_GSM_MODEM_XBEE
#define TINY_GSM_USE_WIFI true

/// NIST "time" protocol port (RFC 868 / daytime-style time service).
#define NIST_PORT 37

// size of NIST response byte from time server
#define NIST_RESPONSE_BYTES 4

#include "modem.h"
#include <StreamDebugger.h>
#include <TinyGsmClient.h>

class DigiXbeeS6B : public Modem {
public:
  DigiXbeeS6B(Stream &xbeeSerial, int8_t powerPin, Stream &debugStream);
  DigiXbeeS6B(Stream &xbeeSerial, int8_t powerPin);

  Client *createClient() override;

  Client *createSecureClient() override;

  bool connectToInternet(const char *ssid, const char *password,
                         uint32_t maxConnectionTime = 60000L) override;

  String getLocalIP() { return _modem.localIP().toString(); }

  bool isInternetAvailable() override;

  uint32_t getNISTTime() override;

  void extraSetupForMQTT() override;

private:
  TinyGsm _modem;
  StreamDebugger _debugger;
};
#endif