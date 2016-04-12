#include <math.h>
#include <SPI.h>
#include <Adafruit_CC3000.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303.h>
#include "utility/debug.h"
#include "utility/socket.h"

///////////////////// VBATT DEF ////////////////////

// #define vBattPin     A0

//////////////////// WIFI DEFINE ///////////////////
#define LEDPIN         A7
#define PORT           23
#define CC3000_CS      10
#define CC3000_IRQ      3
#define CC3000_VBAT     5
#define WLAN_SSID     "hahn-fi"
#define WLAN_PASS     "NBVcvbasd987"
#define WLAN_SECURITY  WLAN_SEC_WPA2
Adafruit_CC3000 cc3000 = Adafruit_CC3000(CC3000_CS,   CC3000_IRQ, 
                                         CC3000_VBAT, SPI_CLOCK_DIVIDER);
Adafruit_CC3000_Server server(PORT);

bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println("Unable to retrieve the IP Address!\r\n");
    return false;
  }
  else
  {
    Serial.print("\r\nIP Addr: "); cc3000.printIPdotsRev(ipAddress);
    Serial.print("\r\nNetmask: "); cc3000.printIPdotsRev(netmask);
    Serial.print("\r\nGateway: "); cc3000.printIPdotsRev(gateway);
    Serial.print("\r\nDHCPsrv: "); cc3000.printIPdotsRev(dhcpserv);
    Serial.print("\r\nDNSserv: "); cc3000.printIPdotsRev(dnsserv);
    Serial.println('\r');
    return true;
  }
}

//////////////////// COMPASS DEFINE ///////////////////

Adafruit_LSM303 mag;

void displaySensorDetails(void)
{
  // sensor_t sensor;
  // mag.getSensor(&sensor);
  // Serial.println("------------------------------------");
  // Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  // Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  // Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  // Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" uT");
  // Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" uT");
  // Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" uT");  
  // Serial.println("------------------------------------");
  // Serial.println("");
  // delay(500);
}

//////////////////// SETUP ////////////////////////

void blink() {
  digitalWrite(LEDPIN, LOW);
  delay(100);
  digitalWrite(LEDPIN, HIGH);
}

void fatal() {
  while(true) {
    blink();
    delay(100);
  }
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Starting wifi-compass ...\n")); 
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  
//////////////////// WIFI SETUP ///////////////////

  Serial.println("\nInitializing wifi ...");

  if (!cc3000.begin()) {
    Serial.println("Couldn't begin(! Check your wiring?");
    while(1);
  }
  blink();
  Serial.println("Connecting to access point...");
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println("Failed!");
    while(1);
  }
  blink();
  Serial.println("Connected to AP, requesting DHCP ...");
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }
  blink();
  Serial.println("Have DHCP, printing details ...\n");
  while (! displayConnectionDetails()) {
    delay(1000);
  }
  server.begin();
  Serial.println("Listening for clients ...\n");
  digitalWrite(LEDPIN, LOW);
  
//////////////////// COMPASS SETUP ///////////////////

// (x,y: 1100/gauss, z: 980/gauss)
  mag.setMagGain(mag.LSM303_MAGGAIN_1_3); 
  
  if(!mag.begin()) {
    Serial.println("no magnometer detected");
    while(1);
  }
  TWBR = 12;  // 400Khz
  displaySensorDetails();
}

//////////////////// LOOP ////////////////////////

void prt3(Adafruit_CC3000_ClientRef client, float f, bool eol) {
  static char headingStr[7];
  headingStr[0] = ' ';
  if (f < 0) {
    f = -f;
    headingStr[0]  = '-';
  }
  headingStr[4] = '\r';
  headingStr[5] = '\n';
  headingStr[6] = 0;
  if (!eol) {
    headingStr[4] = ' ';
    headingStr[5] = ' ';
  }
  int i;
  uint16_t int_f = round(f);
  for (i = 3; i >= 1; i--) {
    headingStr[i] = '0' + int_f % 10;
    int_f /= 10;
  }
  if (headingStr[0] == ' ')
    for (i=1; i<3; i++) {
      if (headingStr[i] == '0')  headingStr[i] = ' '; else break;
    }
  client.write(headingStr, 6);
}

// #define numData 10
// float min = 9e9;
// float max = -9e9;
// int   dataIdx = 0;
// int   totalSamples = 0;
// float data[numData];

void loop(void) {
  // double heading;
  
//////////////////// COMPASS LOOP ////////////////////////
  mag.read();
  mag.magData.z = round(mag.magData.z * (1100.0/980.0));
  
  Serial.print(" "); Serial.print(mag.magData.x);
  Serial.print(" "); Serial.print(mag.magData.y);
  Serial.print(" "); Serial.println(mag.magData.z);
  
  // Serial.print("Accel X: "); Serial.print((int)mag.accelData.x); Serial.print(" ");
  // Serial.print("Y: "); Serial.print((int)mag.accelData.y);       Serial.print(" ");
  // Serial.print("Z: "); Serial.println((int)mag.accelData.z);     Serial.print(" ");

  // heading = 180 * (1 + -atan2(event.magnetic.y, event.magnetic.z) / M_PI);
  // // Serial.println(heading);
  // data[dataIdx++] = heading;
  // dataIdx %= numData;
  // totalSamples++;
  // if (heading < min) min = heading;
  // if (heading > max) max = heading;

//////////////////// WIFI LOOP ////////////////////////

  bool newClient = false;
  static int clientIdx = -1;
  
  if (clientIdx == -1) 
    clientIdx = server.availableIndex(&newClient);
    
  if (clientIdx != -1) {
    Adafruit_CC3000_ClientRef 
      client = server.getClientRef(clientIdx);
    if (newClient) {
      // client.fastrprintln("hello client");
      Serial.println("hello client");
    }
    // int i;
    // float avg = 0.0;
    // for (i=0; i<numData; i++) avg += data[i];
    // avg /= numData;
    if (client && client.connected()) {
      float vBatt = 5 * (2 * analogRead(vBattPin)/1023.0);
      prt3(client, vBatt, true);  

      blink();
      prt3(client, mag.magData.x, false);  
      prt3(client, mag.magData.y, false);  
      prt3(client, mag.magData.z, true);  
      // prt3(client, min, false);
      // if (totalSamples >= numData) {
      //   prt3(client, avg, true);
      //   blink();
      //   Serial.println(avg);
      // }
      // prt3(client, max, false);
      // if (totalSamples >= numData)
      //   prt3(client, avg-min, true);
      // prt3(client, max-min, false);
      // prt3(client, vBatt, true);
    }
    while (client && client.available()) {
      int ch = client.read();
      if (ch == 'q') {
        Serial.println("quiting");
        client.close();
        clientIdx = -1;
        break;
      }
      // if (ch == 'r') {
      //   Serial.println("resetting");
      //   totalSamples = 0;
      //   // min = 9e9;
      //   // max = -9e9;
      //   client.write('\n');
      // }
    }
  }

  delay(500);
}
