#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

// ---------------- DISPLAY SETUP ----------------
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800

#define TFT_CS   10
#define TFT_DC    3
#define TFT_RST   8

TFT_ILI9163C tft = TFT_ILI9163C(TFT_CS, TFT_DC, TFT_RST);

// ---------------- SD CARD ----------------
#define SD_CS 4
File textFile;

// ---------------- LASER RECEIVER ----------------
const int laserPin = 9;
const int ldrPin   = A0;
int threshold = 500;

volatile bool haveByte = false;
volatile byte receivedByte = 0;

// Buffer for building received text
String receivedText = "";

// ---------------- BUTTON ----------------
#define BUTTON_PIN 2     // Active LOW

// =================================================
//                      SETUP
// =================================================
void setup() {

  pinMode(laserPin, OUTPUT);
  pinMode(ldrPin, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(500000);
  setupTimer1();

  // TFT
  tft.begin();
  tft.fillScreen(BLACK);
  tft.setRotation(1);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);

  // SD Card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD FAILED!");
    tft.setCursor(0,0);
    tft.println("SD FAIL!");
  } else {
    Serial.println("SD OK");
  }

  digitalWrite(laserPin, HIGH);
  Serial.println("Receiver ready...");
}

// =================================================
//                        LOOP
// =================================================
void loop() {

  // Store each incoming ASCII into RAM buffer
  if (haveByte) {
    haveByte = false;

    char c = (char)receivedByte;
    receivedText += c;

    Serial.print("Got: ");
    Serial.println(c);
  }

  // When button pressed → display file
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(200);   // debounce

    saveToSD();
    displayFromSD();
  }
}

// =================================================
//                 WRITE TEXT TO SD
// =================================================
void saveToSD() {
  Serial.println("Saving to SD...");

  SD.remove("received.txt");  // remove old file
  textFile = SD.open("received.txt", FILE_WRITE);

  if (textFile) {
    textFile.print(receivedText);
    textFile.close();
    Serial.println("Saved.");
  } else {
    Serial.println("SD WRITE ERROR!");
  }
}

// =================================================
//            DISPLAY CONTENT OF SD FILE
// =================================================
void displayFromSD() {
  Serial.println("Displaying file...");

  textFile = SD.open("received.txt");
  if (!textFile) {
    Serial.println("SD READ ERROR!");
    return;
  }

  tft.fillScreen(BLACK);          // CLEAR SCREEN
  tft.setCursor(0, 0);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);

  while (textFile.available()) {
    char c = textFile.read();
    tft.print(c);
  }

  textFile.close();
}

// =================================================
//                  TIMER — SYNC + RX
// =================================================
void setupTimer1() {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 1999;        // adjust for bit rate
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11);
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect) {

  static byte sliding = 0;
  static byte dataByte = 0;
  static int bitIndex = 0;
  static bool synced = false;

  int raw = analogRead(ldrPin);
  byte bit = (raw > threshold);

  // ---------- SYNC SEARCH (1010) ----------
  if (!synced) {
    sliding = ((sliding << 1) | bit) & 0b1111;

    if (sliding == 0b1010) {
      synced = true;
      bitIndex = 0;
      dataByte = 0;
    }
    return;
  }

  // ---------- READ BYTE ----------
  dataByte = (dataByte << 1) | bit;
  bitIndex++;

  if (bitIndex == 8) {
    receivedByte = dataByte;
    haveByte = true;

    synced = false;
    sliding = 0;
    bitIndex = 0;
    dataByte = 0;
  }
}
