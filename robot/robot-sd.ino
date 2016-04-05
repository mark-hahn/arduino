#include <Servo.h>
#include <LiquidCrystal.h>
#include <Adafruit_CC3000.h>
#include <Wire.h> // I2C
#include <SPI.h>
#include <SD.h>
#include "utility/debug.h"
#include "utility/socket.h"
#include "Sd2Card.h"


///////////////////// DEF MISC ////////////////////
#define LOOP_FREQ 10 // ms
float maxf (float a, float b) { 
  return a > b ? a : b; 
}
float maxi (int a, int b) { 
  return a > b ? a : b; 
}
float minf (float a, float b) { 
  return a < b ? a : b; 
}
float mini (int a, int b) { 
  return a < b ? a : b; 
}


///////////////////// DEF SD ////////////////////
Sd2Card card;
uint8_t block[128];

void printErr() {
  Serial.print(card.errorCode());
  Serial.print(" "); 
  Serial.println(card.errorData());
  while(1) delay(1000); 
}


///////////////////// DEF LED ////////////////////
#define LED_PIN    7
void ledOn(){digitalWrite(LED_PIN, HIGH);}
void ledOff(){digitalWrite(LED_PIN, LOW);}


///////////////////// DEF DISPLAY ////////////////////
//                RS  E  D4  D5  D6  D7
LiquidCrystal lcd( 8, 2, A0, A1, A2,  7 );


///////////////////// DEF VBATT ////////////////////
#define vBattFreqSec 10
#define vBattPin     A3
#define vMax 1.5
#define vMin 1.0
int vBattFreq      = vBattFreqSec * 1000 / LOOP_FREQ;
int vBattFreqCount = vBattFreq;

void chkVbatt() {
  float vBatt;
  int pct;
  if (vBattFreqCount++ == vBattFreq) {
  	vBattFreqCount = 0;
	vBatt = 5 * (2 * analogRead(vBattPin)/1023.0) / 5;
	pct = (int) (99 * (maxf(vMin,minf(vBatt,vMax))-vMin) / (vMax-vMin));
	lcd.setCursor( 9, 0);
	lcd.print(vBatt, 1);
	lcd.setCursor(14, 0);
	lcd.print(pct);
  }
}


////////////////////// DEF SERVO ////////////////////
#define lftWheelPin 10  // O1CB  (actually IO pin 6)
#define rgtWheelPin  9  // O1CA
#define lftStill 1500
#define lftBck   1300
#define lftFwd   1700
#define rgtStill 1500
#define rgtBck   1700
#define rgtFwd   1300
#define speedUpdtMs 100
#define write16(regH, regL, val) { \
  int regHV = (val) >> 8;   \
  int regLV = (val) & 0xff; \
  noInterrupts();           \
  regH = regHV;             \
  regL = regLV;             \
  interrupts();             \
}
int lastLftSpeedCount = -100.0;
int lastRgtSpeedCount = -100.0;

void prtSpeed(int x, float speed) {
  int speedInt;
  lcd.setCursor(x, 0);
  if(speed < 0.0) {
    lcd.print('-');
    speed = -speed;
  } else lcd.print(' ');
  speedInt = (int) (99 * minf(1.0, speed));
  if (speedInt < 10) lcd.print(' ');
  lcd.print(speedInt);
}
void lftSpeed(float speed) {
  if (speed != lastLftSpeedCount) {
  	lastLftSpeedCount = speed;
    prtSpeed(0, speed);
    int spdIntl = (int)(lftStill+((lftFwd-lftStill)*speed));
    write16(OCR1BH, OCR1BL, spdIntl/8);
  }
}
void rgtSpeed(float speed) {
  if (speed != lastRgtSpeedCount) {
  	lastRgtSpeedCount = speed;
    prtSpeed(4, speed);
    int spdIntr = (int)(rgtStill+((rgtFwd-rgtStill)*speed));
    write16(OCR1AH, OCR1AL, spdIntr/8);
  }
}
float rSpeed = 1.0;
int   rSpeedFreq = maxi(LOOP_FREQ, speedUpdtMs) / LOOP_FREQ;
int   rSpeedCount = rSpeedFreq;


///////////////////// DEF WIFI ////////////////////////
#define PORT           88
#define CC3000_CS       6 // (actully IO pin 10)
#define CC3000_IRQ      3
#define CC3000_VBAT     5
#define WLAN_SSID     "hahn-fi"
#define WLAN_PASS     "NBVcvbasd987"
#define WLAN_SECURITY  WLAN_SEC_WPA2
Adafruit_CC3000 cc3000 = Adafruit_CC3000(CC3000_CS,   CC3000_IRQ, 
                                         CC3000_VBAT, SPI_CLOCK_DIVIDER);
Adafruit_CC3000_Server server(PORT);

///////////////////// ROUTINES WIFI /////////////////////
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

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  ledOff();

  //////////////////// SETUP LCD ////////////////////////
  lcd.begin(16, 2);


  //////////////////// SETUP SD ////////////////////////
  if (!card.init(SPI_HALF_SPEED, 4)) { Serial.print("Card Init Err: "); printErr(); }
  Serial.print("Card initialized, size: "); 
  Serial.print(card.cardSize()*512.0/1e9);
  Serial.println(" GB");
  
  uint32_t start = micros();
  if (!card.readData(1234, 256, 128, block)) { Serial.print("Card Read Err: "); printErr(); }
  uint32_t end = micros();
  Serial.println(end-start);
  

  //////////////////// SETUP BATT ////////////////////////
  chkVbatt();
  
  
  //////////////////// SETUP WIFI ////////////////////////
  ledOff();
  lcd.print("Waiting ........");
  
  Serial.println("Hello, CC3000!"); 
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  Serial.println("\nInitializing...");

  if (!cc3000.begin()) {
    Serial.println("Couldn't begin(! Check your wiring?");
    while(1);
  }
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println("Failed!");
    while(1);
  }
  Serial.println("Connected!");
  lcd.setCursor(0, 1);
  lcd.print("Connected ......");
  
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
  Serial.println("Listening for connections...");
  
  ledOn();
  lcd.print("Ready ..........");
  
  
  //////////////////// SETUP SERVO ////////////////////////
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;
  TIMSK1 = 0;
  write16(TCNT1H,TCNT1L, 0);
  write16(ICR1H, ICR1L,  20000/8);
  write16(OCR1AH, OCR1AL, 1500/8);
  write16(OCR1BH, OCR1BL, 1500/8);
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(CS11) | _BV(CS10);
  pinMode(lftWheelPin, OUTPUT);
  pinMode(rgtWheelPin, OUTPUT);
  interrupts();
  lftSpeed(1.0);
  rgtSpeed(1.0);
}


void loop() {
  //////////////////// LOOP VBATT ////////////////////////
  chkVbatt();


  //////////////////// LOOP WIFI ////////////////////////
  uint8_t ch;
  static char str[2] = "\0";
  static int pos = 0;
  static boolean newline = true;
   
  Adafruit_CC3000_ClientRef client = server.available();
  if (client && client.available() > 0) {
    ch = client.read();
    client.write(ch);
    if (newline && ch != 10 && ch != 13){
      newline = false;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      pos = 0;
    }
    if (ch == 10 || ch == 13){
      newline = true;
    } else {
      str[0] = ch;
      lcd.setCursor(pos++, 1);
      lcd.print(str);
    }
  }
  
  
  //////////////////// LOOP SERVO ////////////////////////
  if (rSpeedCount++ == rSpeedFreq) {
	  rSpeedCount = 0;
	  rgtSpeed(rSpeed);
	  rSpeed *= 0.99;
  }
  
//  if (++loopCount % 100 == 0) delay (10000);
  delay(LOOP_FREQ);
}

