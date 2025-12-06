// Laser Receiver with aligned ACK pulse

const int sensorPin = A0;
const int laserPin  = 9;
const unsigned int bitDelay = 1000;
const int threshold = 8;

void setup() {
  pinMode(laserPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("Laser receiver ready");
}

void loop() {
  if (detectSync()) {
    sendSyncAck();
    Serial.println("[SYNC DETECTED]");
    receiveMessage();
  }
}

bool detectSync() {
  int pattern = 0;
  while (true) {
    int val = analogRead(sensorPin);
    pattern = ((pattern << 1) | (val > threshold)) & 0xFF;
    if (pattern == 0b10101010) return true;
    delayMicroseconds(bitDelay);
  }
}

void sendSyncAck() {
  digitalWrite(laserPin, HIGH);
  delay(1);                     // Short ACK pulse
  digitalWrite(laserPin, LOW);
  delayMicroseconds(2 * bitDelay); // guard period
  Serial.println("SYNC ACK sent");
}

void receiveMessage() {
  while (true) {
    if (analogRead(sensorPin) < threshold) {   // Start bit
      delayMicroseconds(bitDelay + bitDelay / 2);

      byte data = 0;
      for (int i = 0; i < 8; i++) {
        if (analogRead(sensorPin) > threshold)
          data |= (1 << i);
        delayMicroseconds(bitDelay);
      }

      delayMicroseconds(bitDelay);  // Stop bit

      if (data >= 32 && data <= 126)
        Serial.write(data);
      else
        Serial.print("?");

      sendAckAligned();
    }
  }
}

void sendAckAligned() {
  digitalWrite(laserPin, HIGH);
  delay(1); // short pulse
  digitalWrite(laserPin, LOW);
  delayMicroseconds(2 * bitDelay); // allow line to settle
  Serial.println(" -> ACK sent and aligned");
}
