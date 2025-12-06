const int laserPin = 9;   // receiver laser output
const int ldrPin   = A0;  // receiver detector input

int threshold = 500;

// States
int sync_index = 0;
bool synced = false;

int bit_index = 0;
byte receivedByte = 0;

void setup() {
  pinMode(laserPin, OUTPUT);
  pinMode(ldrPin, INPUT);

  Serial.begin(9600);
  setupTimer1();

  Serial.println("Receiver ready...");
}

void loop() {
  // nothing
}

void setupTimer1() {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 6249;         // 1 Hz interrupt (same as TX)
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12); 
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect) {

  int sensorValue = analogRead(ldrPin);
  int bitRead = (sensorValue > threshold) ? 1 : 0;

  Serial.print("LDR=");
  Serial.print(sensorValue);
  Serial.print("  bit=");
  Serial.println(bitRead);

  // ============================================================
  //              PHASE 1 — Detect SYNC (10101010)
  // ============================================================
  if (!synced) {
    int expectedBit = (sync_index % 2 == 0) ? 1 : 0;

    if (bitRead == expectedBit) {
      sync_index++;
      Serial.print("Matched sync bit ");
      Serial.println(expectedBit);
    } else {
      sync_index = 0;   // restart sync
    }

    if (sync_index == 8) {
      Serial.println("SYNC detected! Turning ON laser for handshake…");
      digitalWrite(laserPin, HIGH);   // send response laser back to transmitter
      synced = true;
      bit_index = 0;
      receivedByte = 0;
    }

    return;
  }

  // ============================================================
  //         PHASE 2 — RECEIVE DATA BYTE (LSB FIRST)
  // ============================================================
  if (bit_index < 8) {
    receivedByte >>= 1;
    if (bitRead == 1) {
      receivedByte |= 0x80;  // MSB becomes 1 after shift
    }

    Serial.print("Received data bit: ");
    Serial.println(bitRead);
    bit_index++;
    return;
  }

  // ============================================================
  //      PHASE 3 — FULL BYTE RECEIVED → PRINT & RESET
  // ============================================================
  Serial.print("Received ASCII: ");
  Serial.print(receivedByte);
  Serial.print("  Character: ");
  Serial.println((char)receivedByte);

  // Reset to listen for next transmission
  synced = false;
  sync_index = 0;
  digitalWrite(laserPin, LOW);   // turn off handshake laser

  Serial.println("Waiting for next SYNC...\n");
}
