const int sensorPin = A0;
const int threshold = 550;     // night darkness
const unsigned int bitDelay = 10000;  
int n;
byte data;
void setup() {
  Serial.begin(115200);
  Serial.println("Laser receiver ready");
}

void loop() {
  // Wait for sync pattern
  if (detectSync()) {
    n = 1;
    data = 0;
    Serial.println("\n[SYNC DETECTED]");
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

void receiveMessage() {
  while (n) {
    // Wait for start bit (LOW)
    if (analogRead(sensorPin) < threshold) {
      delayMicroseconds(bitDelay+ bitDelay / 10000); // move to middle of first data bit

      for (int i = 0; i < 8; i++) {
        int val = analogRead(sensorPin);
        Serial.print((val > threshold)?1:0);
        if (val > threshold) 
         data |= (1 << i);
        delayMicroseconds(bitDelay);
      }
      Serial.print("\n");
      // Stop bit (ignore)
      delayMicroseconds(bitDelay);

      if (data >= 32 && data <= 126){
        Serial.print(data);
        Serial.write(data); // printable ASCII
      }
      else
        Serial.print("?");
      Serial.print("\n");
      n -= 1;
    }
  }
}
