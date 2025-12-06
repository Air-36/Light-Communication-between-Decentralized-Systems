// Laser Transmitter with edge-aligned ACK handshake

const int laserPin = 9;
const int recvPin  = A0;
const unsigned int bitDelay = 1000;  // microseconds per bit
const int threshold = 8;
const char *message = "HELLO LASER LINK";

void setup() {
  pinMode(laserPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("Laser transmitter ready");
}

void loop() {
  while (!performSyncHandshake()) {
    Serial.println("Resending SYNC...");
    delay(200);
  }

  const char *p = message;
  while (*p) {
    sendByte(*p);
    waitForAckAligned();   // improved alignment
    p++;
  }

  Serial.println("\nMessage sent successfully!");
  delay(3000);
}

//---------------- PROTOCOL ----------------//

bool performSyncHandshake() {
  sendSync();
  Serial.println("SYNC sent, waiting for ACK...");
  unsigned long start = millis();
  while (millis() - start < 500) {
    if (analogRead(recvPin) > threshold) {
      // Wait for ACK to finish
      while (analogRead(recvPin) > threshold);
      delayMicroseconds(2 * bitDelay); // guard period
      Serial.println("SYNC ACK received!");
      return true;
    }
  }
  return false;
}

void sendSync() {
  for (int i = 0; i < 8; i++) {
    digitalWrite(laserPin, i % 2);   // 10101010
    delayMicroseconds(bitDelay);
  }
  digitalWrite(laserPin, LOW);
  delay(10);
}

void sendByte(byte b) {
  Serial.print("TX: "); Serial.write(b); Serial.println();

  digitalWrite(laserPin, LOW);               // Start bit
  delayMicroseconds(bitDelay);

  for (int i = 0; i < 8; i++) {              // Data bits
    digitalWrite(laserPin, (b >> i) & 1);
    delayMicroseconds(bitDelay);
  }

  digitalWrite(laserPin, HIGH);              // Stop bit
  delayMicroseconds(bitDelay);
}

void waitForAckAligned() {
  unsigned long startTime = millis();
  while (millis() - startTime < 500) {
    if (analogRead(recvPin) > threshold) {
      // ACK detected → wait for it to drop
      while (analogRead(recvPin) > threshold);
      delayMicroseconds(2 * bitDelay); // guard time
      Serial.println("ACK aligned, next byte...");
      return;
    }
  }
  Serial.println("ACK timeout, resyncing...");
  performSyncHandshake();  // recover safely
}
