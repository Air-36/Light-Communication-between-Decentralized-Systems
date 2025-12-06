// Receiver (RX)
// laserPin: output driving RX laser to send ACK (pin 9)
// ldrPin:   analog input sensing TX laser (pin A0)
// bit rate: 1 kHz (1 ms tick)

const uint8_t laserPin = 9;
const uint8_t ldrPin   = A0;
const uint16_t ADC_THRESHOLD = 500; // tune to your environment
char finalByte;
x

// ---- receive state machine ----
volatile bool inReception = false;
volatile bool synced = false;
volatile uint8_t bitIndex = 0;
volatile uint8_t dataByte = 0;

// ack generation: RX will emit a single 1-tick ACK when it first sees continuous HIGH from TX
volatile bool ackEmitRequest = false; // when true, ISR will set laser HIGH for exactly one tick

// detection helper
volatile uint8_t consecutiveHigh = 0; // consecutive ticks laser sensed HIGH

void setup() {
  Serial.begin(500000);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);
  pinMode(ldrPin, INPUT);

  setupTimer1_1kHz();
}

void loop() {
  // When a full byte has been collected, print it in main loop to avoid printing in ISR
  if (!inReception && synced == false && bitIndex == 8) {
    // We will never leave bitIndex==8 inside ISR (RX resets bitIndex after printing),
    // but let's implement a safe check: (we'll use a volatile 'dataByte' copy trick).

    // For safety and simplicity: we'll poll a snapshot flag by checking a volatile receiveComplete flag.
  }

  // Non-blocking: print when a new byte arrives (we use a simple handshake via a volatile flag)
  static bool lastPrinted = false;
  static uint8_t lastByte = 0;
  // We'll signal completion by setting a one-tick flag inside ISR via bitIndex==8 and clearing bitIndex there.
  // To keep code compact: use a small shared buffer with a flag.
  static volatile bool rx_ready = false;
  static volatile uint8_t rx_ready_byte = 0;

  // Copy from volatile to local (atomic-ish)
  noInterrupts();
  bool ready = rx_ready;
  uint8_t b = rx_ready_byte;
  if (ready) { rx_ready = false; rx_ready_byte = 0; }
  interrupts();

  if (ready) {
    Serial.print("ASCII: ");
    Serial.print((char)b);
    Serial.print(" HEX: ");
    Serial.println(b, HEX);
  }
}

// Timer1 setup for 1 kHz tick
void setupTimer1_1kHz() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 1999;                // (16e6 / (64 * 1000)) - 1
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11); // prescaler 64
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

// small shared flags for printing
volatile bool rx_ready = false;
volatile uint8_t rx_ready_byte = 0;

// ISR: 1 kHz tick. Detect incoming call, emit 1-tick ACK, then capture sync+data
ISR(TIMER1_COMPA_vect) {
  int raw = analogRead(ldrPin);
  bool light = (raw > ADC_THRESHOLD);

  // If not currently in a reception, detect the "interrupt" call (TX holding laser HIGH)
  if (!inReception && !ackEmitRequest && !synced) {
  //  Serial.print("ConsecutiveHigh: ");
  //  Serial.println(consecutiveHigh);
    if (light) {
      consecutiveHigh++;
    } else {
      consecutiveHigh = 0;
    }

    // When we detect at least 1 consecutive tick of HIGH, we treat it as the interrupt call.
    // (TX holds laser HIGH until ACK; one tick detection is enough to respond immediately.)
    if (consecutiveHigh >= 1) {
      // Request to emit a single 1-tick ACK (ISR will drive laser HIGH this same tick)
      ackEmitRequest = true;
      // enter reception next: after ACK we expect preamble and data
      inReception = true;
      synced = false;
      bitIndex = 0;
      dataByte = 0;
      // Reset consecutiveHigh for future calls
      consecutiveHigh = 0;
      // Note: we will emit ACK in this same tick below
    }
  }

  // Emit ACK for exactly one tick (if requested)
  if (ackEmitRequest) {
    digitalWrite(laserPin, HIGH); // ACK ON for this tick
    // Clear the request so next tick we go back LOW
    ackEmitRequest = false;
    // leaving digitalWrite LOW for subsequent ticks is done at end of ISR when not sending bits
    return;
  } else {
    digitalWrite(laserPin, LOW); // ensure laser is LOW except when ACKing or when RX needs to send data (RX does not transmit data here)
  }

  // If in reception, perform sync detection / bit capture
  if (inReception) {
    // We expect TX to immediately start sending preamble 1 0 1 0 right after ACK.
    // Use sliding window to detect preamble 1010, once found start collecting 8 bits.
  
  rxTimeout++;
    if (rxTimeout >= 20) {  // 20 ms timeout
        inReception = false;
        synced = false;
        bitIndex = 0;
        dataByte = 0;
        consecutiveHigh = 0;
        rxTimeout = 0;
        return;
    }
  
    static uint8_t sliding = 0;

    if (!synced) {
      sliding = ((sliding << 1) | (light ? 1 : 0)) & 0x0F;
      if (sliding == 0b1010) {
        synced = true;
        bitIndex = 0;
        dataByte = 0;
      }
      return;
    }

    // synced == true -> collect 8 bits (MSB-first because sender sends LSB-first? In TX above, we sent LSB-first but built send sequence as direct LSB shifting.
    // In the TX sketch the preamble is sent first then data LSB-first but on RX we use same ordering as TX's produced stream.
    // TX sends bits LSB-first — but we appended bits by outputting the LSB then shifting right. On RX we capture bits as they arrive: to rebuild byte LSB-first, we shift right and set MSB? Easiest: capture LSB-first by storing bits into temp and reconstruct similarly.

    // We'll capture LSB-first into dataByte by writing bit into position bitIndex (0..7).
    uint8_t incomingBit = (light ? 1 : 0);
    dataByte |= (incomingBit << bitIndex);
    bitIndex++;
    Serial.print(incomingBit);
    if (bitIndex >= 8) {
      // Reception complete: copy to shared flags for printing in main loop
      rx_ready_byte = dataByte;
      rx_ready = true;
      
      // Reset reception state
      inReception = false;
      synced = false;
      bitIndex = 0;
      finalByte = dataByte;
      dataByte = 0;
      sliding = 0;
    rxTimeout = 0;   // reset timeout

    }
    Serial.println(finalByte);
    return;
  }
  // default: ensure laser is LOW if not ack emitting
  digitalWrite(laserPin, LOW);
}
