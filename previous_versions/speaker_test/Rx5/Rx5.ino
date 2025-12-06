const int ldrPin = A0;
int threshold = 500;  // adjust for your light level

volatile int bit = 0;
volatile bool newBit = false;

int sync_pattern[8] = {1,0,1,0,1,0,1,0};
int sync_buffer[8] = {0};
bool synced = false;
int data_bits[8];
int bit_count = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Receiver ready at 100 Hz...");

  // === Timer1 setup ===
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  
  // 16 MHz / (64 * (2499 + 1)) = 100 Hz (10 ms per bit)
  OCR1A  = 24999;           // full-bit boundary
  OCR1B  = 12499;           // mid-bit sample (5 ms)
  TCCR1B |= (1 << WGM12);  // CTC mode
  TCCR1B |= (1 << CS11) | (1 << CS10);  // prescaler 64
  TIMSK1 |= (1 << OCIE1B); // enable Compare B interrupt (mid-bit)
  interrupts();
}

ISR(TIMER1_COMPB_vect) {
  int sensorValue = analogRead(ldrPin);
  bit = (sensorValue < threshold) ? 1 : 0;
  newBit = true;
}

void loop() {
  if (newBit) {
    newBit = false;
    Serial.print("Sampled bit: ");
    Serial.println(bit);

    if (!synced) {
      // shift in new bit to sync buffer
      for (int i = 0; i < 7; i++)
        sync_buffer[i] = sync_buffer[i + 1];
      sync_buffer[7] = bit;

      // check if buffer matches sync pattern
      bool match = true;
      for (int i = 0; i < 8; i++)
        if (sync_buffer[i] != sync_pattern[i]) match = false;

      if (match) {
        Serial.println("\n=== SYNC PATTERN DETECTED ===");
        byte sync_byte = 0;
        for (int i = 0; i < 8; i++)
          sync_byte |= (sync_buffer[i] << i);
        Serial.print("Sync bits (LSB→MSB): ");
        for (int i = 7; i >= 0; i--) Serial.print(sync_buffer[i]);
        Serial.println();
        Serial.print("Sync byte (bin): ");
        Serial.println(sync_byte, BIN);
        Serial.print("Sync as char: ");
        Serial.println((char)sync_byte);
        Serial.println("=============================\n");

        synced = true;
        bit_count = 0;
      }
    } 
    else {
      data_bits[bit_count++] = bit;
      if (bit_count == 8) {
        byte received = 0;
        for (int i = 0; i < 8; i++)
          received |= (data_bits[i] << i);

        Serial.println("=== DATA BYTE RECEIVED ===");
        Serial.print("Data bits (LSB→MSB): ");
        for (int i = 7; i >= 0; i--) Serial.print(data_bits[i]);
        Serial.println();
        Serial.print("Data byte (bin): ");
        Serial.println(received, BIN);
        Serial.print("ASCII char: ");
        Serial.println((char)received);
        Serial.println("===========================\n");

        // Reset to look for next sync
        synced = false;
        bit_count = 0;
        for (int i = 0; i < 8; i++) sync_buffer[i] = 0;
        Serial.println("Waiting for next SYNC...");
      }
    }
  }
}
