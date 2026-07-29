#ifndef SRC_MODEMS_DIGIXBEES6B_H_
#define SRC_MODEMS_DIGIXBEES6B_H_

#define TINY_GSM_MODEM_XBEE
#define TINY_GSM_USE_WIFI true
#include "modem.h"
#include <TinyGsmClient.h>

// NIST "time" protocol (37)
#define NIST_PORT 37

// size of NIST response byte from time server
#define NIST_RESPONSE_BYTES 4


class DigiXbeeS6B:public Modem{
    public:
        DigiXbeeS6B(Stream &serial,int8_t powerPin);

        Client *createClient() override ;

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