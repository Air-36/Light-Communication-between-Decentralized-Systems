#include <Keypad.h>

const int ldrPin = A0;

int threshold = 500;
byte b = 'h';
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char hexaKeys[ROWS][COLS] = {
  {'0','1','2','3'},
  {'4','5','6','7'},
  {'8','9','A','B'},
  {'C','D','E','F'}
};
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 10}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

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
  char customKey = customKeypad.getKey();
  
  if (customKey){
    b = customKey;
  }

}


void setupTimer1() {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 62499;        
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12);  
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

//
// Interrupt Service routine
//

ISR(TIMER1_COMPA_vect) {
  static int count = 1;
//  Serial.println(b&1);
  if (b != 0){
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
          b = 0;
          count = 1;
        }
    }
  //  else{
  //    b = 'h';
  //    count = 0;
  //  }
  }
  else{
    digitalWrite(laserPin, LOW);
  }
}
