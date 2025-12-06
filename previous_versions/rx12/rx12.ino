const int ldrPin = A0;
const int rxLaserPin = 9;   // RECEIVER'S LASER
int threshold = 500;

volatile int bit = 0;
volatile bool newBit = false;

int sync_pattern[8] = {1,0,1,0,1,0,1,0};
int sync_buffer[8] = {0};
bool synced = false;
int data_bits[8];
int bit_count = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Receiver ready at 10 Hz...");

  pinMode(rxLaserPin, OUTPUT);
  digitalWrite(rxLaserPin, LOW); // laser OFF initially

  // === Timer1 setup for 10 Hz sampling ===
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  
  // 10 Hz bit rate (100 ms per bit)
  OCR1A  = 624;                  // full bit boundary
  OCR1B  = 312;                  // mid-bit sampling (~50 ms)
  TCCR1B |= (1 << WGM12);         // CTC mode
  TCCR1B |= (1 << CS12);          // prescaler 256 (same as TX)
  TIMSK1 |= (1 << OCIE1B);        // sample at mid-bit
  interrupts();
}

ISR(TIMER1_COMPB_vect) {
  int sensorValue = analogRead(ldrPin);
  bit = (sensorValue > threshold) ? 1 : 0;
  newBit = true;
}

void loop() {
  if (newBit) {
    newBit = false;

    Serial.print("Sampled bit: ");
    Serial.println(bit);

    // ===============================
    //        SYNC DETECTION
    // ===============================
    if (!synced) {

      // shift buffer
      for (int i = 0; i < 7; i++)
        sync_buffer[i] = sync_buffer[i + 1];
      sync_buffer[7] = bit;

      // check match
      bool match = true;
      for (int i = 0; i < 8; i++)
        if (sync_buffer[i] != sync_pattern[i]) match = false;

      // ===========================
      //     SYNC FOUND: TURN LASER ON
      // ===========================
      if (match) {
        Serial.println("\n=== SYNC PATTERN DETECTED ===");
        Serial.println("Turning ON receiver laser (ACK)\n");

        digitalWrite(rxLaserPin, HIGH); // ACK LASER ON

        synced = true;
        bit_count = 0;
      }
    } 

    // ===============================
    //        DATA RECEPTION
    // ===============================
    else {
      data_bits[bit_count++] = bit;

      if (bit_count == 8) {

        byte received = 0;
        for (int i = 0; i < 8; i++)
          received |= (data_bits[i] << i);

        Serial.println("=== DATA BYTE RECEIVED ===");
        Serial.print("ASCII char: ");
        Serial.println((char)received);
        Serial.println("===========================\n");

        // reset for next cycle
        synced = false;
        bit_count = 0;
        for (int i = 0; i < 8; i++) sync_buffer[i] = 0;

        // keep laser ON or turn it OFF?
        // OPTION 1: leave ON (recommended handshake)
        // OPTION 2: turn OFF after receiving the byte:
        // digitalWrite(rxLaserPin, LOW);

        Serial.println("Waiting for next SYNC...");
      }
    }
  }
}
