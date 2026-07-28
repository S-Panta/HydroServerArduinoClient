#ifndef SRC_MODEMS_EXPRESSIFESP32_H_
#define SRC_MODEMS_EXPRESSIFESP32_H_

#define TINY_GSM_MODEM_ESP32
#define TINY_GSM_USE_WIFI true

#include "modem.h"
#include <TinyGsmClient.h>

class ExpressifESP32 : public Modem {

    public:
        ExpressifESP32(Stream &serial, int8_t powerPin);

        Client *createClient() override;

        Client *createSecureClient() override;

        bool connectToInternet(
        const char* ssid,
        const char* password,
        uint32_t maxConnectionTime = 60000L
        ) override;
        
        bool isInternetAvailable() override;

        uint32_t getNISTTime() override;

    private:
        TinyGsm _modem;

};

#endif