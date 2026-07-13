#include <WiFi.h>
#define XBEE_PWR 18
#define XbeeSerial Serial1

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(XBEE_PWR, OUTPUT);
  digitalWrite(XBEE_PWR, HIGH);
  delay(2000);
  XbeeSerial.begin(9600);
  delay(3000);


  WiFi.mode(WIFI_STA);  // Initialize WiFi in station mode (doesn't connect yet)

  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // nothing needed here
}