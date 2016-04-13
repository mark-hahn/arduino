#include <math.h>
#include <SPI.h>
#include <Adafruit_CC3000.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303.h>
#include "utility/debug.h"
#include "utility/socket.h"

///////////////////// MISC DEF ////////////////////

#define netPrintStr    client.fastrprint
#define netPrintlnStr  client.fastrprintln
#define vBattPin       A0
#define LEDPIN         A7

//////////////////// WIFI DEFINE ///////////////////
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
    Serial.print("IP Addr: ");  cc3000.printIPdotsRev(ipAddress);
    Serial.println('\r');
    return true;
  }
}

//////////////////// LMS303 DEFINE ///////////////////

Adafruit_LSM303 lsm303;

//////////////////// SETUP ////////////////////////

#define ledOn()  digitalWrite(LEDPIN, LOW)
#define ledOff() digitalWrite(LEDPIN, HIGH)

void blink() {
  ledOn();
  delay(100);
  ledOff();
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
  ledOn();
  
//////////////////// LSM303 SETUP ///////////////////

    lsm303.setMagGain(lsm303.LSM303_MAGGAIN_1_3); 
    
    if(!lsm303.begin()) {
      Serial.println("no magnometer detected");
      while(1);
    }

    TWBR = 12;  // TWI sclk 400Khz

//////////////////// WIFI SETUP ///////////////////

  Serial.println("Initializing wifi ...");

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
  Serial.println("Have DHCP, printing details ...");
  while (! displayConnectionDetails()) {
    delay(1000);
  }
  server.begin();
  Serial.println("Listening for clients ...\n");
  ledOn();
}
//////////////////// LOOP ////////////////////////

void netPrintFloat_3_2(Adafruit_CC3000_ClientRef client, float val) {
  static char headingStr[8];
  int32_t intVal;
  if (val >= 0) {
    intVal = val*100;
    headingStr[0] = ' ';
  } else {
    intVal = -val*100;
    headingStr[0]  = '-';
  }
  headingStr[4] = '.';
  headingStr[7] = 0;
  int i;
  for (i = 6; i >= 5; i--) {
    headingStr[i] = '0' + intVal % 10;
    intVal /= 10;
  }
  for (i = 3; i >= 1; i--) {
    headingStr[i] = '0' + intVal % 10;
    intVal /= 10;
  }
  if (headingStr[0] == ' ')
    for (i=1; i<3; i++) {
      if (headingStr[i] == '0')  headingStr[i] = ' '; else break;
    }
  netPrintStr(headingStr);
}

void loop(void) {
  float heading;
  
//////////////////// COMPASS LOOP ////////////////////////

  lsm303.read();
  lsm303.magData.z = round(lsm303.magData.z * (1100.0/980.0));
  float x = -lsm303.magData.y +  52.0; // (see compass doc for values)
  float y = -lsm303.magData.z + 205.0;
  heading = 360.0 * atan(y / x) / (2.0 * M_PI) + 90.0;
  if (x >= 0.0) heading += 180.0;
  
  

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
    if (client && client.connected()) {
      blink();
      // float vBatt = 5 * (2 * analogRead(vBattPin)/1023.0);
      
      // netPrintFloat_3_2(client, vBatt);   netPrintStr(" ");
      // netPrintFloat_3_2(client, heading); netPrintlnStr("");
      
      netPrintFloat_3_2(client, lsm303.accelData.x); netPrintStr(" ");
      netPrintFloat_3_2(client, lsm303.accelData.y); netPrintStr(" ");
      netPrintFloat_3_2(client, lsm303.accelData.z); netPrintlnStr("");
      
    }
    while (client && client.available()) {
      int ch = client.read();
      if (ch == 'q') {
        Serial.println("quiting");
        client.close();
        clientIdx = -1;
        break;
      }
    }
  }

  delay(500);
}
