#define XbeeSerial Serial1
#define XBEE_PWR 18

long bauds[] = {9600, 19200, 38400, 57600, 115200};
int numBauds = 5;

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(XBEE_PWR, OUTPUT);
  digitalWrite(XBEE_PWR, HIGH);
  delay(2000);
  Serial.println("Power stabilized, starting baud scan...");
}

void loop() {
  for (int i = 0; i < numBauds; i++) {
    Serial.print("\n--- Trying baud: ");
    Serial.println(bauds[i]);

    XbeeSerial.begin(bauds[i]);
    delay(200);
    while (XbeeSerial.available()) XbeeSerial.read();

    XbeeSerial.print("AT\r\n");
    delay(500);

    Serial.print("Response (hex): ");
    if (XbeeSerial.available()) {
      while (XbeeSerial.available()) {
        uint8_t c = (uint8_t)XbeeSerial.read();  
        Serial.print(c, HEX);
        Serial.print(' ');
      }
    } else {
      Serial.print("(nothing)");
    }
    Serial.println();

    XbeeSerial.end();
    delay(300);
  }

  Serial.println("\n=== Scan complete, repeating in 5s ===\n");
  delay(5000);
}