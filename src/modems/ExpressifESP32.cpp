#include <modems/ExpressifESP32.h>

ExpressifESP32::ExpressifESP32(Stream &serial,int8_t powerPin):
    Modem(powerPin),
    _modem(serial) {}

Client* ExpressifESP32::createClient()
{
    return new TinyGsmClient(_modem);
}

Client* ExpressifESP32::createSecureClient()
{
    // todo
    // secure client is not possible in xbee s6b
    return nullptr;
}

bool ExpressifESP32::connectToInternet(const char* ssid, const char* password ,uint32_t maxConnectionTime){
    // make sure the modem was powered on first
    powerUp();
    _modem.init();
 
    if (!_modem.networkConnect(ssid, password)) {
        Serial.println("is this returning false??");
        return false;
    }
    Serial.println("no the networkConnect is working fine");

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

bool ExpressifESP32::isInternetAvailable(){
    return _modem.isNetworkConnected();
}

uint32_t ExpressifESP32::getNISTTime(){
    // opens UDP socket for time server
    // by default, the time server is pool.ntp.org.
    // by default, it returns UTC time
    _modem.NTPServerSync("pool.ntp.org",0);
    // this function waits for 120 seconds by defaults
    _modem.waitForTimeSync(30);
    uint32_t epoch = _modem.getNetworkEpoch(TinyGSM_EpochStart::UNIX);
}