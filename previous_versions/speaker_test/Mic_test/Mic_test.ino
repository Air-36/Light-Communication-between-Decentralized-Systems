// Laser Audio Transmitter
// Mic input on A0, Laser output on D9 (PWM pin)

#define MIC_PIN A0
#define LASER_PIN 9

void setup() {
  pinMode(LASER_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Laser Audio Transmission Started...");
}

void loop() {
  int micValue = analogRead(MIC_PIN);       // 0 - 1023
  byte pwmValue = micValue / 4;             // Convert to 0 - 255
  analogWrite(LASER_PIN, pwmValue);         // Modulate laser brightness
}
