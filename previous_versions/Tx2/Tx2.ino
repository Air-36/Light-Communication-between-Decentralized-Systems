const int laserPin = 9;

const int ldrPin = A0;



const char* message = "HELLO";

int msgIndex = 0;



int bitIndex = 0;        

byte currentByte = 0;



bool synced = false;   // LDR has detected laser



void setup() {

  pinMode(laserPin, OUTPUT);

  Serial.begin(9600);



  digitalWrite(laserPin, HIGH);  // KEEP LASER ON UNTIL SYNC



  setupTimer1();

}



void loop() {

  // Check LDR only until synced

  if (!synced) {

    int val = analogRead(ldrPin);

    if (val < 500) {    // threshold — adjust according to your setup

      Serial.println("SYNC achieved by LDR.");

      synced = true;



      // Now allow ISR to start sending bits

      bitIndex = 0;

      msgIndex = 0;

    }

  }

}



void setupTimer1() {

  cli();

  TCCR1A = 0;

  TCCR1B = 0;

  TCNT1 = 0;



  OCR1A = 624;                

  TCCR1B |= (1 << WGM12);      

  TCCR1B |= (1 << CS12);       

  TIMSK1 |= (1 << OCIE1A);     



  sei();

}



ISR(TIMER1_COMPA_vect) {



  if (!synced) {

    // do NOTHING until sync is achieved

    return;

  }



  // Once synced → laser now transmits bits instead of just staying ON

  if (bitIndex == 0) {

    currentByte = message[msgIndex];

    Serial.print("Sending: ");

    Serial.println((char)currentByte);

  }



  // Send data bit (LSB first)

  int bitToSend = (currentByte >> bitIndex) & 1;

  digitalWrite(laserPin, bitToSend);



  Serial.print("Bit ");

  Serial.print(bitIndex);

  Serial.print(": ");

  Serial.println(bitToSend);



  bitIndex++;



  if (bitIndex == 8) {

    bitIndex = 0;

    msgIndex++;



    if (message[msgIndex] == '\0')

      msgIndex = 0;

  }

}
