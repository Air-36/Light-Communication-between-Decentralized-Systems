const int laserPin = 9;
const int ldrPin   = A0;

int threshold = 500;

volatile bool haveByte = false;
volatile byte receivedByte = 0;

void setup() {
  pinMode(laserPin, OUTPUT);
  pinMode(ldrPin, INPUT);

  Serial.begin(500000);
  setupTimer1();

  Serial.println("Receiver ready...");
  digitalWrite(laserPin, 1);
}

void loop() {
//  if (haveByte) {
//    haveByte = false;
//
//    Serial.print("ASCII: ");
//    Serial.print((char)receivedByte);
//    Serial.print("   HEX: ");
//    Serial.println(receivedByte, HEX);
//  }
}

void setupTimer1() {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 62499;       // adjust for your bit rate
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect) {

  static byte sliding = 0;     // last 4 bits sliding window
  static byte dataByte = 0;    // current receiving byte
  static int bitIndex = 0;
  static bool synced = false;  // true when 1010 found

  // Read bit
  int raw = analogRead(ldrPin);
  byte bit = (raw > threshold);  // 1 or 0
  Serial.print(bit);
  // --- PHASE 1: Looking for 1010 ---
  if (!synced) {

    // Shift sliding window (keep last 4 bits)
    sliding = ((sliding << 1) | bit) & 0b1111; 

    // Check if sliding window == 1010 (binary)
    if (sliding == 0b1010) {
      synced = true;
      bitIndex = 0;
      dataByte = 0;
    }

    return;   // keep searching
  }

  // --- PHASE 2: Once synced, collect 8 bits ---
  dataByte = (dataByte << 1) | bit;
  bitIndex++;

  if (bitIndex == 8) {
    receivedByte = dataByte;
    haveByte = true;

    // Reset for next sync sequence
    synced = false;
    sliding = 0;
    bitIndex = 0;
    dataByte = 0;
  }

  if (haveByte == true){
    Serial.print("ASCII: ");
    Serial.print((char)receivedByte);
    Serial.print("   HEX: ");
    Serial.println(receivedByte, HEX);
    haveByte = false;
  }
}
