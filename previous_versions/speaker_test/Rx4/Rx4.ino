const int ldrPin = A0;
int threshold = 500;   // adjust based on your light level

// --- SYNC PATTERN ---
int sync_pattern[8] = {1,0,1,0,1,0,1,0};
int sync_buffer[8] = {0};

// --- STATE VARIABLES ---
volatile bool synced = false;
volatile int sync_count = 0;
volatile int bit_count = 0;
volatile byte receivedByte = 0;

void setup() {
  Serial.begin(9600);
  setupTimer1();
}

void loop() {
  // Nothing in main loop; ISR handles bit timing.
}

// --- Timer setup for 1 Hz (same as transmitter) ---
void setupTimer1() {
  cli();  // disable interrupts

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 624;           // 1 Hz with prescaler 256
  TCCR1B |= (1 << WGM12);  // CTC mode
  TCCR1B |= (1 << CS12);   // prescaler 256
  TIMSK1 |= (1 << OCIE1A); // enable compare A interrupt

  sei();  // enable interrupts
}

// --- Interrupt runs once per second ---
ISR(TIMER1_COMPA_vect) {
  int sensorValue = analogRead(ldrPin);
  int bit = (sensorValue > threshold) ? 0 : 1;

  Serial.print("Read bit: ");
  Serial.println(bit);

  if (!synced) {
    // Shift sync buffer left
    for (int i = 0; i < 7; i++)
      sync_buffer[i] = sync_buffer[i+1];
    sync_buffer[7] = bit;

    // Check for sync match
    bool match = true;
    for (int i = 0; i < 8; i++) {
      if (sync_buffer[i] != sync_pattern[i]) {
        match = false;
        break;
      }
    }

    if (match) {
      synced = true;
      bit_count = 0;
      receivedByte = 0;
      Serial.println("SYNC detected!");
    }
  }
  else {
    // Collect 8 bits (LSB first)
    receivedByte |= (bit << bit_count);
    bit_count++;

    if (bit_count == 8) {
      Serial.print("Received byte: ");
      Serial.println(receivedByte, BIN);
      Serial.print("As char: ");
      Serial.println((char)receivedByte);
      
      // Reset for next cycle
      synced = false;
      bit_count = 0;
      receivedByte = 0;
      for (int i = 0; i < 8; i++) sync_buffer[i] = 0;
      Serial.println("Waiting for next SYNC...");
    }
  }
}
