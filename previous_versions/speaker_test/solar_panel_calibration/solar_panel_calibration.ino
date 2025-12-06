void setup() {
  // put your setup code here, to run once:
pinMode(A0, INPUT);
pinMode(A1, INPUT);
pinMode(A2, INPUT);
pinMode(A3, INPUT);
Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  float S1, L1, S2, L2;
  S1 = analogRead(A0);
  L1 = analogRead(A1);
  S2 = analogRead(A2);
  L2 = analogRead(A3);
//Serial.print("Solar Panel 1: ");
S1 = S1*100/1024;
Serial.println(S1);
delay(10);
}
