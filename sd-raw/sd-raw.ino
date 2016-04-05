
#include <SPI.h>
#include "Sd2Card.h"

Sd2Card card;
uint8_t block[512];

void printErr() {
  Serial.print(card.errorCode());
  Serial.print(" "); 
  Serial.println(card.errorData());
  while(1) delay(1000); 
}

void setup() {
  Serial.begin(115200);
  
  if (!card.init(SPI_HALF_SPEED, 4)) { Serial.print("Card Init Err: "); printErr(); }
  Serial.print("Card initialized, size: "); 
  Serial.print(card.cardSize()*512.0/1e9);
  Serial.println(" GB");

  if (!card.readBlock(-1, block)) { Serial.print("Card Read Err: "); printErr(); }
  // int i;
  // for (i = 0; i < 512; i++) {
  //   Serial.println(block[i]);
  // }
  
}

void loop() {
}
