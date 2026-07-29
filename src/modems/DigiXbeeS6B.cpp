#include <modems/DigiXbeeS6B.h>


DigiXbeeS6B::DigiXbeeS6B(Stream &serial,int8_t powerPin)
    : Modem(powerPin),
    _modem(serial)
    //   initialize here for tinygsm debug
{
}

Client* DigiXbeeS6B::createClient()
{
    return new TinyGsmClient(_modem);
}

Client* DigiXbeeS6B::createSecureClient()
{
    // secure client is not possible in xbee s6b
    return nullptr;
}

bool DigiXbeeS6B::connectToInternet(const char* ssid, const char* password ,uint32_t maxConnectionTime){
    // make sure the modem was powered on first
    powerUp();
    if (!_modem.networkConnect(ssid, password)) {
        return false;
    }

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

bool DigiXbeeS6B::isInternetAvailable(){
    return _modem.isNetworkConnected();
}

// returns time in utc
uint32_t DigiXbeeS6B::getNISTTime(){
    if(!isInternetAvailable()){
        return 0;
    }
    //the IP address of time-[a,b,c,d]-wwv.nist.gov.
    // xbee can't open udp and tcp at the same time. So tcp connection should be opened.
    // tcp is needed for http/mqtt connection
    const char *nistIP = "132.163.97.1";
    
    Client *client = createClient();
    // sets up the tcp connection
    bool connected = client->connect(nistIP, NIST_PORT);
    if(connected){
        // send something to open TCP connection
        client->println('!');
        uint32_t startTime = millis();
        while(client && client->available()< NIST_RESPONSE_BYTES && millis() - startTime < 5000L){
            // waiting for atleast 5 seconds
        }

        if(client->available()>=NIST_RESPONSE_BYTES){
            uint8_t buffer[NIST_RESPONSE_BYTES];
            client->read(buffer,NIST_RESPONSE_BYTES);
            // stop the tcp connection
            client->stop();
            uint32_t ntpTime = ((uint32_t)buffer[0] << 24) |
                         ((uint32_t)buffer[1] << 16) |
                         ((uint32_t)buffer[2] << 8) | (uint32_t)buffer[3];
            // NTP time counts seconds since 1900-01-01 00:00:00 UTC.
            // Unix time counts seconds since 1970-01-01 00:00:00 UTC.
            // The difference between those two epochs is 2208988800 seconds.
            uint32_t unixTime = ntpTime - 2208988800UL;

            // Mayfly logger has Onboard realtime clock (RTC) (DS3231)
            // DS3231 chip give the number of seconds since January 1, 2000
            // Unix Time, which is the number of seconds since 1/1/1970
            // https://www.envirodiy.org/ds3231-real-time-clock-rtc-date-conversion/
            // DateTime dt(unixTime - 946684800UL);
            uint32_t currentdateTime = unixTime - 946684800UL;
            return currentdateTime;
        } else {
            client->stop();
        } 
    } else {
        return 0;
    } 
}

