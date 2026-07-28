#ifndef SRC_MODEMS_MODEM_H_
#define SRC_MODEMS_MODEM_H_

#include <TinyGsmClient.h>

class Modem {
    public:
     Modem(Stream &serial);

     Client * createClient();
     void begin();
     void connectToWifi();
     void extraSetupForMQTT();
     void extraSetupForHTTPS();
     void getNISTTime() ;

    protected:
     TinyGsmClient *_client;
     TinyGsm *_modem;
};

#endif