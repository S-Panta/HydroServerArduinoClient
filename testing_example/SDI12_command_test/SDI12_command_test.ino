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
uint32_t wake_delay = 2000;              // Sensor wake-up delay

SDI12 mySDI12(dataPin);


void setup() {

  Serial.begin(serialBaud);

  while (!Serial && millis() < 10000L);

    // Power the sensor
  if (powerPin >= 0) {

    Serial.println("Powering up sensor...");

    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);

    Serial.println("Waiting 15 seconds for sensor...");
    delay(15000L);
  }

  Serial.println();
  Serial.println("================================");
  Serial.println("       SDI-12 TERMINAL");
  Serial.println("================================");

  Serial.println("Opening SDI-12 bus...");

  mySDI12.begin();

  delay(500);

  Serial.print("Timeout value: ");
  Serial.println(mySDI12.TIMEOUT);



  Serial.println();
  Serial.println("Sensor ready.");
  Serial.println("Type an SDI-12 command and press Enter.");
  Serial.println();
  Serial.println("Examples:");
  Serial.println("  0!");
  Serial.println("  0I!");
  Serial.println("  0M!");
  Serial.println("  0M2!");
  Serial.println("  0D0!");
  Serial.println();
  Serial.println("--------------------------------");
}


void loop() {

  // Wait for a command from the computer
  if (Serial.available()) {

    // Read the command until Enter
    String command = Serial.readStringUntil('\n');

    // Remove spaces and carriage returns
    command.trim();

    // Don't do anything if the command is empty
    if (command.length() == 0) {
      return;
    }

    Serial.println();

    // Clear previous SDI-12 data
    mySDI12.clearBuffer();

    Serial.print("Command: ");
    Serial.println(command);

    mySDI12.sendCommand(command);
    delay(300);
    if (mySDI12.available()) {
    Serial.println("Immediate response from sensor:");

    while (mySDI12.available()) {
      char c = mySDI12.read();

      if (c == '\n') {
        break;  // End of response
      }

      Serial.write(c);
    }
    }
  }
}