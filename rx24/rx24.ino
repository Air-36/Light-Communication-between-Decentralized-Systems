#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define __CS 10
#define __DC 3
#define __RST 8

TFT_ILI9163C tft = TFT_ILI9163C(__CS, __DC, __RST);

#define SD_CS 4
File logFile;

#define BUTTON_PIN 7
#define CLR_PIN 6

const uint8_t laserPin = 9;
const uint8_t ldrPin   = A0;
const uint16_t ADC_THRESHOLD = 500;
char finalByte;

volatile bool inReception = false;
volatile bool synced = false;
volatile uint8_t bitIndex = 0;
volatile uint8_t dataByte = 0;
volatile uint8_t rxTimeout = 0;

volatile bool ackEmitRequest = false;
volatile uint8_t consecutiveHigh = 0;

volatile bool rx_ready = false;
volatile uint8_t rx_ready_byte = 0;

const int Y_CORRECTION = -32;

//
// SAFETY CHECK FUNCTION FOR FILE
//

void ensureFileExists(const char* filename) {
  digitalWrite(__CS, HIGH);
  digitalWrite(__DC, HIGH);
  delayMicroseconds(50);

  if (!SD.exists(filename)) {
    File f = SD.open(filename, FILE_WRITE);
    if (f) {
      f.println("");  
      f.close();
    }
  }

  digitalWrite(__CS, LOW);
  digitalWrite(__DC, LOW);
  delayMicroseconds(20);
}


// 
//     SETUP
//



void setup() {
  Serial.begin(500000);

  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);
  pinMode(ldrPin, INPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(CLR_PIN, INPUT_PULLUP);
  pinMode(__CS, OUTPUT);
  pinMode(__DC, OUTPUT);

  digitalWrite(__CS, LOW);
  digitalWrite(__DC, LOW);

  tft.begin();
  tft.fillScreen(BLACK);
  tft.setRotation(0);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(5, 50);
  tft.println("OLED Ready");

  Serial.println("Initializing SD...");

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  digitalWrite(__CS, HIGH);
  digitalWrite(__DC, HIGH);
  delayMicroseconds(50);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed!");
    tft.setCursor(5, 100);
    tft.setTextColor(RED);
    tft.println("SD FAIL");
  } else {
    Serial.println("SD ready.");
    tft.setCursor(5, 100);
    tft.setTextColor(GREEN);
    tft.println("SD Ready");
  }
  ensureFileExists("shri.txt");

  digitalWrite(__CS, LOW);
  digitalWrite(__DC, LOW);

  setupTimer1_1kHz();
}

// 
//      MAIN LOOP
// 


void loop() {

  noInterrupts();
  bool ready = rx_ready;
  char b = rx_ready_byte;
  if (ready) {
    rx_ready = false;
    rx_ready_byte = 0;
  }
  interrupts();

  if (ready) {
    Serial.print("RECV: ");
    Serial.println(b);

    digitalWrite(__CS, HIGH);
    digitalWrite(__DC, HIGH);
    delayMicroseconds(50);

    logFile = SD.open("shri.txt", FILE_WRITE);
    if (logFile) {
      logFile.print(b);
      logFile.flush();
      logFile.close();
      Serial.println("WRITE OK");
    } else {
      Serial.println("File open failed!");
    }

    digitalWrite(__CS, LOW);
    digitalWrite(__DC, LOW);
    delayMicroseconds(20);
  }

if (digitalRead(CLR_PIN) == LOW) {
    delay(150); 

    digitalWrite(__CS, HIGH);
    digitalWrite(__DC, HIGH);
    delayMicroseconds(50);

    if (SD.exists("shri.txt")) {
        SD.remove("shri.txt");              
        File f = SD.open("shri.txt", FILE_WRITE);
        if (f) {
            f.close();                      
            Serial.println("shri.txt cleared");
        } else {
            Serial.println("Could not recreate file!");
        }
    } else {
        Serial.println("File does not exist.");
    }

    // restore TFT
    digitalWrite(__CS, LOW);
    digitalWrite(__DC, LOW);
    delayMicroseconds(20);
}

  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(150);
    displaySDContent();
  }
}


//
//   DISPLAY 
// 


void displaySDContent() {
  tft.fillScreen(BLACK);

  digitalWrite(__CS, HIGH);
  digitalWrite(__DC, HIGH);
  delayMicroseconds(50);

  logFile = SD.open("shri.txt");
  if (!logFile) {
    digitalWrite(__CS, LOW);
    digitalWrite(__DC, LOW);

    tft.setTextSize(2);
    tft.setTextColor(RED);
    tft.setCursor(5, 40 + Y_CORRECTION);
    tft.println("NO FILE!");
    return;
  }

  String content = "";
  const size_t MAX_READ = 512;
  size_t readCount = 0;

  while (logFile.available() && readCount < MAX_READ) {
    content += (char)logFile.read();
    readCount++;
  }
  logFile.close();

  digitalWrite(__CS, LOW);
  digitalWrite(__DC, LOW);
  delayMicroseconds(20);

  if (content.length() == 0) {
    tft.setTextSize(2);
    tft.setTextColor(YELLOW);
    tft.setCursor(5, 50 + Y_CORRECTION);
    tft.println("Empty");
    return;
  }

  const int ts = 2;
  const int charW = 6 * ts;
  const int charH = 8 * ts;

  int textWidth = content.length() * charW;
  int x = (128 - textWidth) / 2;
  int y = (128 - charH) / 2;
  y += Y_CORRECTION;
  if (y < 0) y = 0;

  tft.fillScreen(BLACK);
  tft.setTextSize(ts);
  tft.setTextColor(YELLOW);
  tft.setCursor(x, y);
  tft.print(content);
}


// 
//   TIMER 1 INIT
// 


void setupTimer1_1kHz() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 399;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11);
  TIMSK1 |= (1 << OCIE1A);
  sei();
}


// 
//   ISR 
// 


ISR(TIMER1_COMPA_vect) {
  int raw = analogRead(ldrPin);
  bool light = (raw > ADC_THRESHOLD);

  if (!inReception && !ackEmitRequest && !synced) {
    if (light) consecutiveHigh++;
    else       consecutiveHigh = 0;

    if (consecutiveHigh >= 1) {
      ackEmitRequest = true;
      inReception = true;
      synced = false;
      bitIndex = 0;
      dataByte = 0;
      consecutiveHigh = 0;
    }
  }

  if (ackEmitRequest) {
    digitalWrite(laserPin, HIGH);
    ackEmitRequest = false;
    return;
  } else {
    digitalWrite(laserPin, LOW);
  }

  if (inReception) {
    rxTimeout++;
    if (rxTimeout >= 20) {
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

    uint8_t incomingBit = (light ? 1 : 0);
    dataByte |= (incomingBit << bitIndex);
    bitIndex++;
    Serial.print(incomingBit);

    if (bitIndex >= 8) {
      rx_ready_byte = dataByte;
      rx_ready = true;

      inReception = false;
      synced = false;
      bitIndex = 0;
      finalByte = dataByte;
      dataByte = 0;
      sliding = 0;
      rxTimeout = 0;
    }

    Serial.println(finalByte);
    return;
  }

  digitalWrite(laserPin, LOW);
}
