#include <Arduino.h>

#define XbeeSerial Serial1
#define XBEE_PWR 18

const int32_t modemBaud = 57600;

String sendATCommand(String cmd, uint32_t timeout_ms = 2000) {
  while (XbeeSerial.available()) {
    XbeeSerial.read();
  }

  Serial.print(">> ");
  Serial.println(cmd);

  XbeeSerial.print(cmd);
  XbeeSerial.print("\r\n");

  String response = "";
  uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    while (XbeeSerial.available()) {
      char c = XbeeSerial.read();
      response += c;
      start = millis(); 
    }
    if (response.endsWith("OK\r\n") || response.endsWith("ERROR\r\n")) {
      break;
    }
  }

  Serial.print("<< ");
  Serial.println(response);
  Serial.println("....................................");

  return response;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("esp32 AT command test");

  pinMode(XBEE_PWR, OUTPUT);
  digitalWrite(XBEE_PWR, HIGH);

  XbeeSerial.begin(modemBaud);
  delay(3000);
  Serial.println("powered up module");

  sendATCommand("AT");
// sendATCommand("AT+GMR");
// sendATCommand("AT+CIPMUX?");
// sendATCommand("AT+CIPSTATUS");
// sendATCommand("AT+CMD?");
sendATCommand("AT+CIPSTA?");
delay(2000);
sendATCommand("AT+CIPSTART=\"TCP\",\"192.168.0.103\",1883");
sendATCommand("AT+CIPMODE?");
sendATCommand("AT+PING=\"192.168.0.103\"");

  // sendATCommand("AT+GMR");
  // sendATCommand("AT+CWMODE?");
  // sendATCommand("AT+CIPSTA?");

  Serial.println("done");
}

void loop() {
}