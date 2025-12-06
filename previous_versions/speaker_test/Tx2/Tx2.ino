const int laserPin = 9;
const unsigned int bitDelay = 100000; 

const char *message = "HELLO";

byte b = B11100110;

void setup() {
  pinMode(laserPin, OUTPUT);
  setupTimer1();
  Serial.begin(115200);
  Serial.println("Laser transmitter ready");
}

void loop() {
//  sendSync();       
//  sendString(message);
//  delay(3000);       
}

//void sendSync() {
//  for (int i = 0; i < 8; i++) {
//    digitalWrite(laserPin, i % 2); // 10101010
//    delayMicroseconds(bitDelay);
//  }
//  digitalWrite(laserPin, LOW);
//  delayMicroseconds(bitDelay);
//}

//void sendString(const char *msg) {
//  while (*msg) {
//    sendByte(*msg++);
//  }
//}

//void sendByte(byte b) {
//  // Start bit
//  digitalWrite(laserPin, LOW);
//  delayMicroseconds(bitDelay);
//
//  // Data bits
//  for (int i = 0; i < 8; i++) {
//    digitalWrite(laserPin, (b >> i) & 1);
//    delayMicroseconds(bitDelay);
//  }
//
//  // Stop bit
//  digitalWrite(laserPin, HIGH);
//  delayMicroseconds(bitDelay);
//}


void setupTimer1() {
  // Disable interrupts during setup
  cli();
  
  // Reset Timer1 Control Registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;  // Initialize counter value
  
  // Set compare match register for 1 Hz increments (1 second)
  // Formula: OCR1A = (16MHz / (prescaler * desired_frequency)) - 1
  // For 1 Hz with prescaler 256: (16000000 / (256 * 1)) - 1 = 62499
  OCR1A = 624;
  
  // Turn on CTC mode (Clear Timer on Compare Match)
  TCCR1B |= (1 << WGM12);
  
  // Set prescaler to 256
  TCCR1B |= (1 << CS12);  // CS12 = 1, CS11 = 0, CS10 = 0
  
  // Enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  
  // Enable global interrupts
  sei();
}


ISR(TIMER1_COMPA_vect) {
//  ledState = !ledState;
  b = b >> 1;
  digitalWrite(laserPin, b & 1);
}
