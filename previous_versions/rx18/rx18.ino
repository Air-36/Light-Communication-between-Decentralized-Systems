#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

// ------------ OLED DEFINITIONS ------------
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

// ------------ SD DEFINITIONS ------------
#define SD_CS 4
File logFile;

// ------------ BUTTON ------------
#define BUTTON_PIN 7

// ------------ RECEIVER ORIGINAL VARIABLES ------------
const uint8_t laserPin = 9;
const uint8_t ldrPin   = A0;
const uint16_t ADC_THRESHOLD = 500;
char finalByte;

volatile bool inReception = false;
volatile bool synced = false;
volatile uint8_t bitIndex = 0;
volatile uint8_t dataByte = 0;
volatile uint8_t rxTimeout = 0;   // timeout counter for inReception

volatile bool ackEmitRequest = false;
volatile uint8_t consecutiveHigh = 0;

volatile bool rx_ready = false;
volatile uint8_t rx_ready_byte = 0;

// ------------ DISPLAY ADJUSTMENT ------------
const int Y_CORRECTION = -32; // compensate ILI9163C variant vertical offset

// ------------------------------------------------------
//                    SETUP
// ------------------------------------------------------
void setup() {
  Serial.begin(500000);

  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);
  pinMode(ldrPin, INPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Prepare TFT control pins so we can manually isolate it when required
  pinMode(__CS, OUTPUT);
  pinMode(__DC, OUTPUT);
  // Let the display start enabled (CS LOW, DC LOW)
  digitalWrite(__CS, LOW);
  digitalWrite(__DC, LOW);

  // ---- OLED INIT ----
  tft.begin();
  tft.fillScreen(BLACK);
  tft.setRotation(0);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(5, 50);
  tft.println("OLED Ready");

  // ---- SD INIT ----
  Serial.println("Initializing SD...");

  // Make sure SD CS is an output and idle high
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // Fully disable TFT before calling SD.begin() to avoid SPI contention
  digitalWrite(__CS, HIGH);   // deactivate TFT CS
  digitalWrite(__DC, HIGH);   // put DC high so TFT won't latch commands
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

  // Re-enable TFT for normal operation
  digitalWrite(__CS, LOW);
  digitalWrite(__DC, LOW);

  setupTimer1_1kHz();
}

void loop() {

  // -------- Handle received byte from ISR ---------
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

    // -------- SD Write: fully isolate TFT from SPI first --------
    digitalWrite(__CS, HIGH);    // disable TFT CS
    digitalWrite(__DC, HIGH);    // ensure DC won't cause TFT activity
    delayMicroseconds(50);

    logFile = SD.open("shri.txt", FILE_WRITE);
    if (logFile) {
      logFile.print(b);
      logFile.flush();    // ensure data is written to card
      logFile.close();
      Serial.println("WRITE OK");
    } else {
      Serial.println("File open failed!");
    }

    // Re-enable TFT
    digitalWrite(__CS, LOW);
    digitalWrite(__DC, LOW);
    delayMicroseconds(20);
  }

  // -------- Display SD contents on button press --------
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(150);
    displaySDContent();
  }
}



// ------------------------------------------------------
//                   OLED DISPLAY FUNCTION
// ------------------------------------------------------
void displaySDContent() {
  // clear screen first
  tft.fillScreen(BLACK);

  // Isolate TFT from SPI and open SD file
  digitalWrite(__CS, HIGH);
  digitalWrite(__DC, HIGH);
  delayMicroseconds(50);

  logFile = SD.open("shri.txt");
  if (!logFile) {
    // re-enable TFT to show error
    digitalWrite(__CS, LOW);
    digitalWrite(__DC, LOW);

    tft.setTextSize(2);
    tft.setTextColor(RED);
    tft.setCursor(5, 40 + Y_CORRECTION);
    tft.println("NO FILE!");
    return;
  }

  // Read file contents into a buffer (limit to avoid huge allocations)
  String content = "";
  const size_t MAX_READ = 512; // adjust if needed
  size_t readCount = 0;
  while (logFile.available() && readCount < MAX_READ) {
    content += (char)logFile.read();
    readCount++;
  }
  logFile.close();

  // Re-enable TFT before drawing
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

  // Manual centering WITHOUT getTextBounds()
  // For Adafruit_GFX font: default char box is 6x8 at size=1. With setTextSize(2): width=6*2, height=8*2.
  const int ts = 2;
  const int charW = 6 * ts;
  const int charH = 8 * ts;

  // For simplicity, display single-line centered. If too long it will overflow.
  int textWidth = content.length() * charW;
  int x = (128 - textWidth) / 2;
  int y = (128 - charH) / 2;

  // Apply Y correction for this ILI9163C variant
  y += Y_CORRECTION;
  if (y < 0) y = 0;

  tft.fillScreen(BLACK);
  tft.setTextSize(ts);
  tft.setTextColor(YELLOW);
  tft.setCursor(x, y);
  tft.print(content);
}



// ------------------------------------------------------
//                   TIMER 1 INIT (1 kHz)
// ------------------------------------------------------
void setupTimer1_1kHz() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 399;    // keep your value as requested
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11);
  TIMSK1 |= (1 << OCIE1A);
  sei();
}



// ------------------------------------------------------
//                  ORIGINAL ISR (UNTOUCHED)
// ------------------------------------------------------
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
