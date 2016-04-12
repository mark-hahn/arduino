#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303.h>

Adafruit_LSM303 mag;

void setup() {
  
  Serial.begin(9600);
  
  // Try to initialise and warn if we couldn't detect the chip
  if (!mag.begin())
  {
    Serial.println("Oops ... unable to initialize the LSM303. Check your wiring!");
    while (1);
  }
}

void loop() 
{
  mag.read();
  Serial.print("Accel X: "); Serial.print((int)mag.accelData.x); Serial.print(" ");
  Serial.print("Y: "); Serial.print((int)mag.accelData.y);       Serial.print(" ");
  Serial.print("Z: "); Serial.println((int)mag.accelData.z);     Serial.print(" ");
  Serial.print("Mag X: "); Serial.print((int)mag.magData.x);     Serial.print(" ");
  Serial.print("Y: "); Serial.print((int)mag.magData.y);         Serial.print(" ");
  Serial.print("Z: "); Serial.println((int)mag.magData.z);       Serial.print(" ");
  delay(1000);
}
