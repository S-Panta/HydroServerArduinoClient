#define XbeeSerial Serial1
#define XbeePower 18
#define TINY_GSM_MODEM_XBEE

#include <TinyGsmClient.h>
#define TINY_GSM_USE_WIFI true

// for normal mode
TinyGsm modem(XbeeSerial);

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(XbeePower, OUTPUT);
  digitalWrite(XbeePower, HIGH);  

  XbeeSerial.begin(9600);
  delay(1000);
  
  Serial.println("trying tinygsm");
  delay(2000);

  String responseid = modem.sendATGetString(GF("AI"));
  Serial.println(responseid);
  delay(2000);
  
  enterCommandMode();
  sendATCommand("ATID");   // SSID
  sendATCommand("ATEE");   // Encryption type enabled (0=none,2=WPA,3=WPA2,4=WEP)
  sendATCommand("ATAI");   // Association indication (0 = connected)
  sendATCommand("ATMY");   // Current IP address
  sendATCommand("ATIP");   // IP protocol (TCP/UDP)
  sendATCommand("ATDL");
  sendATCommand("ATPK");
  sendATCommand("AT");

  exitCommandMode();
}

void loop() {}

void enterCommandMode() {
  delay(1200);              // guard time before
  XbeeSerial.print("+++");
  delay(1200);              // guard time after
  String resp = readXbeeResponse();
  Serial.print("Enter CMD mode: ");
  Serial.println(resp);     // should print "OK"
}

void exitCommandMode() {
  XbeeSerial.println("ATCN");
  Serial.println(readXbeeResponse());
}

void sendATCommand(String cmd) {
  XbeeSerial.println(cmd);
  delay(200);
  String resp = readXbeeResponse();
  Serial.print(cmd);
  Serial.print(" -> ");
  Serial.println(resp);
}

String readXbeeResponse() {
  String result = "";
  unsigned long start = millis();
  while (millis() - start < 500) {
    while (XbeeSerial.available()) {
      char c = XbeeSerial.read();
      result += c;
    }
  }
  result.trim();
  return result;
}