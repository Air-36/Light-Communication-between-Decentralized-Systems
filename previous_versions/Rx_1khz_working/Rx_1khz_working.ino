const int laserPin = 9;
const int ldrPin   = A0;

int threshold = 500;

volatile bool haveSample = false;
volatile int rawSample = 0;

void setup() {
  pinMode(laserPin, OUTPUT);
  pinMode(ldrPin, INPUT);

  Serial.begin(500000);   // MUST use fast baud
  setupTimer1();

  Serial.println("Receiver ready...");
  digitalWrite(laserPin, 1);
}

void loop() {
  if (haveSample) {
    haveSample = false;

//    int bit = (rawSample < threshold) ? 1 : 0;
//    Serial.print("bit=");
//    Serial.println(bit);

    // put your sync/data logic here (NOT in ISR)
  }
}

void setupTimer1() {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // For 100 Hz: period = 10 ms
  // 16 MHz / 256 prescaler = 62500 ticks per sec
  // 10 ms = 625 ticks
  OCR1A = 1999;    

  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11);
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect) {
  static int count = 0;
  static byte char_ascii = 0;
  rawSample = analogRead(ldrPin);
  
//  haveSample = true;   // signal main loop
  rawSample = rawSample>500;
  
  Serial.print(rawSample);
}
