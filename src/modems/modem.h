#ifndef SRC_MODEMS_MODEM_H_
#define SRC_MODEMS_MODEM_H_

#include <Arduino.h>
#include <Client.h>

class Modem {
    public:
        Modem(int8_t powerPin): _powerPin(powerPin){};

        // destructor
        virtual ~Modem() = default;

        virtual Client* createClient() = 0;

        // Optional: not all modems support secure clients
        virtual Client* createSecureClient()
        {
            return nullptr;
        }

        virtual void powerUp(){
            pinMode(_powerPin,OUTPUT);
            digitalWrite(_powerPin,HIGH);
        };

        virtual bool connectToInternet(const char* ssid, const char* password,uint32_t maxConnectionTime = 60000L)=0;

        virtual bool isInternetAvailable() = 0;

        virtual uint32_t getNISTTime() = 0;


        // // todo: implement this for debug output
        // String getModemName();

        //  void extraSetupForMQTT();
        //  void extraSetupForHTTPS();

    private:
        // by default, mayfly 1.1 has pin 18
        int8_t _powerPin=18;

};

#endif