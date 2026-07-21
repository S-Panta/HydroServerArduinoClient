// #include <Wire.h>
// #include <RTClib.h>

// RTC_DS3231 rtc;

// // #include <Udp.h>

// // Udp udpClient;

// // 60 millisecond for update frequency 
// // setUpdateInterval can be used to change later
// // NTPClient timeClient(udpClient, "north-america.pool.ntp.org", -6 * 3600,60000);

// String getISO8601Time(const DateTime& t) {
//   char buf[25];
//   snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
//            t.year(),
//            t.month(),
//            t.day(),
//            t.hour(),
//            t.minute(),
//            t.second());
//   return String(buf);
// }

// void setup() {
//   Serial.begin(115200);
//   Serial.println("rtc for mayfly");
//   Wire.begin();

//   if (!rtc.begin()) {
//     Serial.println("Couldn't find DS3231");
//     while (1) delay(10);
//   }
// }

// void loop() {


//     DateTime now = rtc.now();
//     // uint32_t epoch = now.unixtime();
//     Serial.println(getISO8601Time(now));

//     // Serial.print(now.year());  Serial.print('-');
//     // Serial.print(now.month()); Serial.print('-');
//     // Serial.print(now.day());   Serial.print(' ');
//     // Serial.print(now.hour());  Serial.print(':');
//     // Serial.print(now.minute()); Serial.print(':');
//     // Serial.println(now.second());
//     delay(2000);
// }


#include <ModularSensors.h>
#include <modems/DigiXBeeWifi.h>
#include <Sodaq_DS3231.h>

HardwareSerial& modemSerial = Serial1;

const char* ssid     = "USU-guest";
const char* password = NULL;

const int8_t modemVccPin     = 18;
const int8_t modemStatusPin  = 19;
const bool   useCTSforStatus = true;
const int8_t modemResetPin   = 20;
const int8_t modemSleepRqPin = 23;

DigiXBeeWifi modemXBWF(&modemSerial, modemVccPin, modemStatusPin,
                       useCTSforStatus, modemResetPin, modemSleepRqPin, ssid,
                       password);



void printDateTime(DateTime dt) {
    char buf[25];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
             dt.year(), dt.month(), dt.date(),
             dt.hour(), dt.minute(), dt.second());
    Serial.println(buf);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    modemSerial.begin(9600);

    Wire.begin();
    rtc.begin();

    Serial.println("Powering modem...");
    modemXBWF.modemWake();
    Serial.print("Local IP: ");
    Serial.println(modemXBWF.gsmModem.localIP());

    Serial.println("Connecting to WiFi...");
    if (modemXBWF.connectInternet()) {
        Serial.println("Requesting NIST time...");
        uint32_t epoch = modemXBWF.getNISTTime();
        if (epoch == 0) {
            Serial.println("Failed to get time.");
        } else {
            Serial.print("Epoch: ");
            Serial.println(epoch);
            rtc.setEpoch(epoch);       
            Serial.print("RTC: ");
            printDateTime(rtc.now());
        }
        modemXBWF.disconnectInternet();
    } else {
        Serial.println("Failed to connect to WiFi.");
    }

    modemXBWF.modemSleep();
}

void loop() {
    printDateTime(rtc.now());
    delay(1000);
}
