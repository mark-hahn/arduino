
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>

////////////////////////// SD CARD DEF ///////////////////////////

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

const int chipSelect = 4;
    
void fatal(const String& msg) {
  Serial.print("Fatal error: "); Serial.println(msg);
  while(1);
}

////////////////////////// DAC DEF ///////////////////////////

Adafruit_MCP4725 dac;

////// BUFFER ACCESS FUNCTIONS /////

uint8_t buffer[256];
uint8_t *bufPtr;

bool isEqStr (const char* str, int len) {
  bool eq = true;
  uint8_t *ptr = bufPtr;
  bufPtr += len;
  for (; eq && *ptr && ptr != bufPtr; ptr++, str++) {
    eq = eq && (*ptr == *str);
  }
  return eq;
}
uint16_t getUInt16() {
  bufPtr += 2;
  return (bufPtr[-1] << 8 | bufPtr[-2]);
}
uint32_t getUInt32() {
  uint32_t num = 0;
  uint8_t *ptr;
  for (ptr = bufPtr+3; ptr >= bufPtr; ptr--) num = (num << 8) | *ptr;
  bufPtr += 4;
  return num;
}

void setup() {

  Serial.begin(115200);
  
  pinMode(4, OUTPUT);
  if (!SD.begin(4)) fatal("SD initialization failed");
  File myFile = SD.open("TEST.WAV");
  if (!myFile) fatal("error opening TEST.WAV");
  
  myFile.read(buffer, 256);
  bufPtr = buffer;
  
  if (!isEqStr("RIFF", 4)) fatal("not .wav file");
  uint8_t *eof = (uint8_t *)(getUInt32()+8);
  if (!isEqStr("WAVE", 4))  fatal("no WAVE");

  // chunk loop
  while (bufPtr < eof) {
    Serial.print("getting chunk at "); Serial.print((uint32_t)(bufPtr - buffer));
    Serial.print(" of "); Serial.println((uint32_t)(eof-buffer));
    char chunkType;
    if (isEqStr("fmt ", 4)) chunkType = 'f';
    else {
      bufPtr -= 4;
      if (isEqStr("data", 4)) chunkType = 'd';
      else {
        bufPtr -= 4;
        if (isEqStr("fact", 4)) chunkType = 'a';
        else fatal("chunk type unknown");
      }
    }
    int chunkHdrLen = getUInt32();
    Serial.print("chunkType: ");   Serial.println(chunkType);
    Serial.print("chunkHdrLen: "); Serial.println(chunkHdrLen);
    if (chunkType == 'f') {
      uint16_t formatTag = getUInt16();   //1 if uncompressed Microsoft PCM audio
      Serial.print("formatTag: "); Serial.println(formatTag);
      // ushort  wChannels;       //Number of channels
      // uint    dwSamplesPerSec; //Frequency of the audio in Hz
      // uint    dwAvgBytesPerSec;//For estimating RAM allocation
      // ushort  wBlockAlign;     //Sample frame size in bytes
      // uint    dwBitsPerSample; //Bits per sample
    }
    while(1);
  }
  
  // Serial.println("Playing");
  // while (myFile.available()) {
  //   myFile.read(buffer, sizeof(buffer));
  //   uint8_t i;
  //   for (i=0; i < sizeof(buffer); i++) {
  //     dac.setVoltage(buffer[i], false);
  //   } 
  // }
  // Serial.println("Done playing\r\n");
}

void loop(void) {
}
