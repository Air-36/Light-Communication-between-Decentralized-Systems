const int ldrPin = A0;

int threshold = 500;
byte b = 'h';

const int laserPin = 9;
void setup() {
  // put your setup code here, to run once:
  setupTimer1();
  pinMode(ldrPin, INPUT);
  pinMode(laserPin, OUTPUT);
  Serial.begin(500000);
}

void loop() {
  // put your main code here, to run repeatedly:

}


void setupTimer1() {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 1999;        // 1 Hz (10 Hz would be 62499/10 etc)
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11);  // prescaler 256
  TIMSK1 |= (1 << OCIE1A);

  sei();
}



ISR(TIMER1_COMPA_vect) {
  static int count = 1;
//  Serial.println(b&1);
  if (count<5){
    digitalWrite(laserPin, count%2);
    count++;
  }
  else if(count<13){
      if (b&1) {
      PORTB |= (1 << PB1);    // Pin 9 HIGH
    } else {
      PORTB &= ~(1 << PB1);   // Pin 9 LOW
    }
    b = b>>1;
    count++;
      if (count == 13){
        b = 'h';
        count = 1;
      }
  }
//  else{
//    b = 'h';
//    count = 0;
//  }
}
