// Arduino Speaker Test Code
// Connect speaker/buzzer +ve to pin 8, -ve to GND

int speakerPin = 9;

void setup() {
  // nothing to initialize
}

void loop() {
  // Play tones of different frequencies
  tone(speakerPin, 500);   // 500 Hz
  delay(500);
  tone(speakerPin, 1000);  // 1 kHz
  delay(500);
  tone(speakerPin, 1500);  // 1.5 kHz
  delay(500);
  noTone(speakerPin);      // Stop tone
  delay(500);
}
