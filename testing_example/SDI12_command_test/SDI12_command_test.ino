#include <SDI12.h>

#ifndef SDI12_DATA_PIN
#define SDI12_DATA_PIN 7
#endif

#ifndef SDI12_POWER_PIN
#define SDI12_POWER_PIN 22
#endif

uint32_t serialBaud = 57600;           // USB serial baud rate
int8_t   dataPin    = SDI12_DATA_PIN;  // SDI-12 data bus
int8_t   powerPin   = SDI12_POWER_PIN; // Sensor power pin

SDI12 mySDI12(dataPin);


// Read one SDI-12 reply line. Returns as soon as <LF> arrives,
// or "" if nothing complete arrives before the timeout.
String readReply(uint32_t timeoutMs) {
  String reply = "";
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    if (mySDI12.available()) {
      char c = mySDI12.read();
      if (c == '\n') return reply;   // end of reply
      if (c != '\r') reply += c;
    }
  }
  return reply;  // timed out (may be empty or partial)
}


void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);

  Serial.println();
  Serial.println("================================");
  Serial.println("       SDI-12 TERMINAL");
  Serial.println("================================");

  Serial.println("Opening SDI-12 bus...");
  mySDI12.begin();
  delay(500);

  if (powerPin >= 0) {
    Serial.println("Powering up sensor...");
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(15000);
  }

  Serial.println();
  Serial.println("Sensor ready. Type an SDI-12 command and press Enter.");
  Serial.println("Examples: 0!  0I!  0M1!  0D0!");
  Serial.println("--------------------------------");
}


void loop() {
  if (!Serial.available()) return;

  String command = Serial.readStringUntil('\n');
  command.trim();
  if (command.length() == 0) return;
  if (!command.endsWith("!")) command += "!";

  Serial.print("Command: ");
  Serial.println(command);

  mySDI12.clearBuffer();
  mySDI12.sendCommand(command);

  // Immediate reply (sensors answer within milliseconds)
  String reply = readReply(3000);
  Serial.print("Reply:   ");
  Serial.println(reply.length() ? reply : "(no response)");

  // For M commands, the ack is "atttn": wait up to ttt seconds
  // for the service request that says the data is ready.
  bool isMeasure = command.length() >= 3 && command.charAt(1) == 'M';
  if (isMeasure && reply.length() >= 5) {
    uint32_t waitSec = reply.substring(1, 4).toInt();
    int      nVals   = reply.substring(4).toInt();

    Serial.print("Measuring: ");
    Serial.print(nVals);
    Serial.print(" value(s), up to ");
    Serial.print(waitSec);
    Serial.println(" s...");

    String sr = readReply(waitSec * 1000UL + 500);
    if (sr.length()) {
      Serial.println("Data ready. Send aD0! (e.g. 0D0!)");
    } else {
      Serial.println("No service request; try 0D0! anyway.");
    }
  }

  Serial.println("--------------------------------");
}