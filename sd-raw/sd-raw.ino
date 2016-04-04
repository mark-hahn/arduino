
#include <SPI.h>


void setup() {
  /* Implementation of class used to create `SDCard` object. */
  card.init(SPI_HALF_SPEED, 4, 11, 12, 13);
  
}

void loop() {
}
