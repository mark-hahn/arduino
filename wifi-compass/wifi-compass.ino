#include <SPI.h>
#include <math.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_CC3000.h>
#include <Adafruit_LSM303_U.h>
#include "utility/debug.h"
#include "utility/socket.h"

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

//////////////////// SETUP ////////////////////////

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Starting wifi-compass ...\n")); 
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  
  //////////////////// WIFI SETUP ///////////////////

  Serial.println("\nInitializing wifi ...");

  if (!cc3000.begin()) {
    Serial.println("Couldn't begin(! Check your wiring?");
    while(1);
  }
  Serial.println("Connecting to access point...");
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println("Failed!");
    while(1);
  }
  Serial.println("Connected!");
  
  Serial.println("Request DHCP");
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  /* Display the IP address DNS, Gateway, etc. */  
  while (! displayConnectionDetails()) {
    delay(1000);
  }
  // Start listening for connections
  server.begin();
  Serial.println("Listening for clients ...\n");
  
//////////////////// COMPASS SETUP ///////////////////

    mag.setMagRate(LSM303_MAGRATE_220);
    mag.enableAutoRange(true);
    if(!mag.begin()) {
      Serial.println("no magnometer detected");
      while(1);
    }
    TWSR = 0;
    TWBR = 2;  // 12 => 736, 2 => 570
    displaySensorDetails();
}

//////////////////// LOOP ////////////////////////

void loop(void) {
  double heading;

//////////////////// COMPASS LOOP ////////////////////////

  sensors_event_t event; 
  mag.getEvent(&event);
  // θ = arctan ( y / x )
  heading = 360.0 * (-atan2(event.magnetic.y, event.magnetic.z) / (2*M_PI));
  while (heading  <   0.0) heading += 360.0;
  while (heading >= 360.0) heading -= 360.0;
  Serial.println(heading);

//////////////////// WIFI LOOP ////////////////////////

  static char headingStr[6];
  headingStr[3] = '\r';
  headingStr[4] = 0;
  headingStr[5] = 0;
  bool newClient = false;
  static int clientIdx = -1;
  
  if (clientIdx == -1) clientIdx = server.availableIndex(&newClient);
  if (clientIdx != -1) {
    Adafruit_CC3000_ClientRef client = server.getClientRef(clientIdx);
    if (newClient) client.fastrprintln("hello client");
    if (client && client.connected()) {
      uint16_t int_heading = round(heading);
      int i;
      for (i = 0; i < 3; i++) {
        headingStr[2-i] = '0' + int_heading % 10;
        int_heading /= 10;
      }
      client.write(headingStr, 6);
      Serial.println(headingStr);
    }
    while (client && client.available()) {
      if (client.read() == 'q') {
        client.close();
        clientIdx = -1;
        break;
      }
    }
  }
    
  
  // for (clientIdx=0; clientIdx<3; clientIdx++) {
  //   
  //   Adafruit_CC3000_ClientRef client = server.getClientRef(clientIdx);
  //   if (client && client.connected()) {
  //     Serial.print("idx: "); Serial.println(clientIdx);
  //     if (newClient) client.fastrprintln("hello client");
  //   } 
  // }
  // if (!clientIdx) clientIdx = server.availableIndex(&newClient);
  // Serial.println(server.availableIndex(&newClient));
  
  // if (clientIdx) {
  //    client = server.available();
  //   if (newClient) {
  //     Serial.print("client connected: "); Serial.println(clientIdx);
  //     connected = true;
  //   }
  //   if (client.available() && client.read() == 'q') {
  //     client.close();
  //   } else {
  //     uint16_t int_heading = round(heading);
  //     for (i = 0; i < 3; i++) {
  //       headingStr[3-i] = '0' + int_heading % 10;
  //       int_heading /= 10;
  //     }
  //     client.write(headingStr, 6);
  //     Serial.println(headingStr);
  //   }
  // } else {
  //   if (!client.connected) {
  //     Serial.println("client disconnected");
  //     connected = false;
  //   }
  // }
  delay(500);
}
