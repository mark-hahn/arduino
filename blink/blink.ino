
void setup() {
  Serial.begin(115200);
  Serial.println("hello world2");
  pinMode(7, OUTPUT);
}

void loop() {
  digitalWrite(7, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(7, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);              // wait for a second
}
