#include "Modem.h"

Modem::Modem(Stream &serial)
    : _modem(serial),
      _client(modem)
{
}

Client *Modem::createClient()
{
    return &client;
}