#ifndef SRC_MODEMS_DIGIXBEE_H_
#define SRC_MODEMS_DIGIXBEE_H_

#define TINY_GSM_MODEM_XBEE
#include "modem.h"

class DigiXbee:public Modem{
    public:
        DigiXbee(Stream &serial);
};

#endif