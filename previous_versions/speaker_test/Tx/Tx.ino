const int laserPin = 9;
const unsigned int bitDelay = 1000; 

const char *message = "UUUUU";

void setup() {
  pinMode(laserPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("Laser transmitter ready");
}

void loop() {
  sendSync();       
  sendString(message);
  delay(3000);       
}

void sendSync() {
  for (int i = 0; i < 8; i++) {
    digitalWrite(laserPin, i % 2); // 10101010
    delayMicroseconds(bitDelay);
  }
  digitalWrite(laserPin, LOW);
  delayMicroseconds(bitDelay);
}

void sendString(const char *msg) {
  while (*msg) {
    sendByte(*msg++);
  }
}

void sendByte(byte b) {
  // Start bit
  digitalWrite(laserPin, LOW);
  delayMicroseconds(bitDelay);

  // Data bits
  for (int i = 0; i < 8; i++) {
    digitalWrite(laserPin, (b >> i) & 1);
    delayMicroseconds(bitDelay);
  }

  // Stop bit
  digitalWrite(laserPin, HIGH);
  delayMicroseconds(bitDelay);
}
