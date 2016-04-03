
#define lftWheelPin 10  // O1CB  (actually IO pin 6)
#define rgtWheelPin  9  // O1CA

uint32_t countPerUs (int scale, uint32_t us) {
  return us/(scale/16);
}
#define write16(regH, regL, val) { \
  regH = val >> 8;                 \
  regL = val & 0xff;               \
}

void setup() {
  noInterrupts();           // disable all interrupts
  
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;
  TIMSK1 = 0;
  write16(TCNT1H,TCNT1L, 0);
  
  // period -- count 2500 @   64x = 50Hz (20ms)
  write16(ICR1H, ICR1L, countPerUs(64, 20000) / 2);
  // duty 1500us = 1250
  write16(OCR1AH, OCR1AL, countPerUs(64, 1450)/2);
  write16(OCR1BH, OCR1BL, countPerUs(64, 1550)/2);

//  // period -- count 7812 @ 1024x = 1Hz
//  write16(ICR1H, ICR1L, countPerUs(1024, 1000000)/2);
//  // 50% duty = 3906
//  write16(OCR1AH, OCR1AL, countPerUs(1024, 100000)/2);
//  write16(OCR1BH, OCR1BL, countPerUs(1024, 500000)/2);

  // mode 10 (PWM, Phase Correct, ICR1), WGM13=1, WGM12=0, WGM11=1, WGM10=0 
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
  
  // 64x prescaler, CS12=0, CS11=1, CS10=1
  TCCR1B = _BV(WGM13) | _BV(CS11) | _BV(CS10);

//   // 1024x prescaler, CS12=1, CS11=0, CS10=1
//  TCCR1B = _BV(WGM13) | _BV(CS12) | _BV(CS10);
 
  pinMode(lftWheelPin, OUTPUT);
  pinMode(rgtWheelPin, OUTPUT);

  interrupts();
}

void loop() {
	
}
