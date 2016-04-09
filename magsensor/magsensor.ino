#include <math.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>

Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345);

void displaySensorDetails(void)
{
  sensor_t sensor;
  mag.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" uT");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" uT");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" uT");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void setup(void) 
{
#ifndef ESP8266
  while (!Serial);     // will pause Zero, Leonardo, etc until serial console opens
#endif
  Serial.begin(115200);
  Serial.println("Magnetometer Test"); Serial.println("");
  
  mag.setMagRate(LSM303_MAGRATE_220);
  mag.enableAutoRange(true);
  if(!mag.begin())
  {
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  TWSR = 0;
  TWBR = 2;  // 12 => 736, 2 => 570
  displaySensorDetails();
}

void loop(void) 
{
  sensors_event_t event; 
  
  // uint16_t start = micros();  
  mag.getEvent(&event);
  // uint16_t end = micros();  
  // Serial.println(end-start);

  // r = √ ( x*x + y*y )
  // θ = arctan ( y / x )
  
  double degrees = 360.0 * (-atan2(event.magnetic.y, event.magnetic.z) / (2*M_PI));
  while (degrees  <   0.0) degrees += 360.0;
  while (degrees >= 360.0) degrees -= 360.0;
  Serial.print("heading: "); Serial.println(degrees);
 
  /* Display the results (magnetic vector values are in micro-Tesla (uT)) */
  // Serial.print("X: "); Serial.print(event.magnetic.x); Serial.print("  ");
  // Serial.print("Y: "); Serial.print(event.magnetic.y); Serial.print("  ");
  // Serial.print("Z: "); Serial.print(event.magnetic.z); Serial.print("  ");Serial.println("uT");
  delay(500);
}