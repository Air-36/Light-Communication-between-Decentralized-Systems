const int laserPin = 9;      // Laser output pin
const char* message = "HELLO";   // Message to send

int msgIndex = 0;            // Which character in message
byte currentByte = 0;
int bitIndex = 0;

bool sendingSync = true;     // Start with sync burst

void setup() {
  pinMode(laserPin, OUTPUT);
  Serial.begin(9600);

  setupTimer1();
  Serial.println("Transmitter ready. Sending SYNC...");
}

void loop() {
  // Nothing needed in loop
}

void setupTimer1() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 624;                // ~20 Hz match your receiver
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12);      // prescaler 256
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect) {

  // First send SYNC → continuous HIGH for 500 ms
  static int syncCount = 0;

  if (sendingSync) {
    digitalWrite(laserPin, HIGH);
    syncCount++;

    if (syncCount >= 10) {   // ~500 ms @ 20 Hz
      sendingSync = false;
      Serial.println("SYNC sent. Starting message...");
    }
    return;
  }

  // If message finished
  if (message[msgIndex] == '\0') {
    msgIndex = 0;  // restart message
  }

  // Load next byte if starting a new character
  if (bitIndex == 0)
    currentByte = message[msgIndex];

  // Extract bit LSB-first
  int bit = (currentByte >> bitIndex) & 1;

  if (bit == 1)
    digitalWrite(laserPin, HIGH);
  else
    digitalWrite(laserPin, LOW);

  Serial.print("Sent bit ");
  Serial.print(bitIndex);
  Serial.print(": ");
  Serial.println(bit);

  bitIndex++;

  // Finished sending full byte?
  if (bitIndex == 8) {
    Serial.print("Sent Byte: ");
    Serial.print(message[msgIndex]);
    Serial.print(" (");
    Serial.print(currentByte, BIN);
    Serial.println(")");

    bitIndex = 0;
    msgIndex++;
  }
}
