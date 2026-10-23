/*
  This file is part of HydroServerArduinoClient library.
  Sabin Panta
*/

#ifndef SRC_MODEMS_ESP32BEE_H_
#define SRC_MODEMS_ESP32BEE_H_

#define TINY_GSM_MODEM_ESP32
#define TINY_GSM_USE_WIFI true

#include "modem.h"
#include <TinyGsmClient.h>

#include <StreamDebugger.h>

class ESP32Bee : public Modem {

public:
  ESP32Bee(Stream &xbeeSerial, int8_t powerPin, Stream &debugStream);
  ESP32Bee(Stream &xbeeSerial, int8_t powerPin);

  Client *createClient() override;

  Client *createSecureClient() override;

  bool connectToInternet(const char *ssid, const char *password,
                         uint32_t maxConnectionTime = 60000L) override;

  bool isInternetAvailable() override;

  // Normally,the baud rate is either 115200 or 57600 but sometime it can be
  // different Use this function to detect the baud rate of your espressif bee
  // baud Don't use this in production
  uint32_t detectBaud(HardwareSerial &serial);

  // String getLocalIP() { return _modem.localIP().toString(); }

  uint32_t getNISTTime() override;

  void extraSetupForHTTPS(const char *host, const int port);

  void extraSetupForMQTT() override;

  String getMacAddress();

  int8_t getSignalQuality();
  String getLocalIP() { return _modem.getLocalIP(); }

  String getModemInfo() { return _modem.getModemInfo(); }
  String getFirmwareVersion();

private:
  TinyGsm _modem;
  StreamDebugger _debugger;
};

#endif