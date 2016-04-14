
#include "PetitFS.h"
#include "PetitSerial.h"
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <MemoryFree.h>

// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

PetitSerial PS;
#define Serial PS
FATFS fs;

////////////////////////// SD CARD DEF ///////////////////////////

#define BUFLEN 512

void fatal(__FlashStringHelper* msg) {
  Serial.print(F("Fatal error: ")); Serial.println(msg);
  while(1);
}

void dumpHex(uint8_t* buf, int len) {
  int cnt = 0;
  uint8_t* max = buf + len;
  while(buf < max) {
    uint8_t* max2 = buf + 16;
    for(; buf < max2 && cnt < len; buf++, cnt++) {
      String hd = String(*buf, HEX); 
      if (hd.length() < 2) hd = String("0" + hd);
      Serial.print(hd); Serial.print(F(" "));
      if (cnt % 4 == 3) Serial.print(F(" "));
    }
    Serial.println();
  }
}

// ////////////////////////// DAC DEF ///////////////////////////

Adafruit_MCP4725 dac;

// ////////////////////////// SD FILE READ ///////////////////////////
// 
uint8_t buffer[2*BUFLEN];
uint8_t *bufPtr = buffer;
#define buffer1 (buffer)
#define buffer2 (buffer + BUFLEN)

// shared with TWI interrupts
volatile uint8_t *bufWritePtr = buffer1;
volatile uint8_t *bufReadPtr  = buffer1;

uint32_t fileLength = 1000000000;
uint16_t channels;
uint32_t framesPerSec;
uint32_t avgBytesPerSec;
uint16_t frameSize;
uint16_t bitsPerSample;
uint32_t numSamples;

uint32_t totalBytesRead = 0;
bool fileReadActive = false;

void readFile() {
  Serial.println(F("readFile"));
  Serial.print(F("freeMemory: ")); Serial.println(freeMemory());
  if (!fileReadActive) return;
  // noInterrupts();
  bool bufEmpty = (bufWritePtr == bufReadPtr);
  // interrupts();
  
  Serial.print(F("bufEmpty: ")); Serial.println(bufEmpty);
  Serial.print(F("bufWritePtr: ")); Serial.println((uint32_t)bufWritePtr);
  
  while (!bufEmpty &&
         ((bufWritePtr == buffer1 && bufReadPtr  < buffer2) ||
          (bufWritePtr == buffer2 && bufReadPtr >= buffer2))) {}
  
  uint16_t bytesRead;
  if (pf_read(buffer, sizeof(buffer), &bytesRead)) fatal(F("file read"));
  Serial.print(F("bytesRead: ")); Serial.println(bytesRead);
  // buffer[bytesRead] = 0;
  // Serial.print((char*)buffer);
  // dumpHex(buffer, bytesRead);
  
  totalBytesRead += bytesRead;
  Serial.print(F("totalBytesRead: ")); Serial.println(totalBytesRead);
  
  // noInterrupts();
  bufWritePtr += bytesRead;
  if (bufWritePtr >= buffer2+BUFLEN) bufWritePtr = buffer1;
  // interrupts();
  
  if (bytesRead < BUFLEN ||
       (fileLength < 1000000000 && 
        totalBytesRead >= fileLength)) {
    fileReadActive = false;
    Serial.print(F("done reading file, bytes read: ")); 
    Serial.println(totalBytesRead);
  }
}

void openFile(const char* fileName) {
  Serial.print(F("opening file: ")); Serial.println(fileName);
  Serial.print(F("freeMemory: ")); Serial.println(freeMemory());
  if (pf_open(fileName)) fatal(F("fs open"));
  fileReadActive = true;
  // noInterrupts();
  bufWritePtr = buffer1;
  bufReadPtr  = buffer1;
  // interrupts();
  readFile();
}

////////////////////// BUFFER ACCESS FUNCTIONS //////////////////////

bool isEqStr (const char* str, int len) {
  bool eq = true;
  uint8_t *ptr = bufPtr;
  bufPtr += len;
  for (; eq && *ptr && ptr != bufPtr; ptr++, str++) {
    eq = eq && (*ptr == *str);
  }
  return eq && ptr == bufPtr;
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


////////////////////// SETUP //////////////////////

void setup() {
  Serial.begin(115200);
  Serial.print(F("freeMemory: ")); Serial.println(freeMemory());

  if (pf_mount(&fs)) fatal(F("sd & os mount"));

  openFile("TEST.WAV");
  // openFile(F("TEST.TXT"));
  
  if (!isEqStr("RIFF", 4)) fatal(F("not .wav file"));
  fileLength = getUInt32() + 8;
  if (!isEqStr("WAVE", 4)) fatal(F("no WAVE"));
  
  uint8_t *eof = bufPtr + fileLength;
  
  // chunk loop
  while (bufPtr < eof) {
    String chunkType = String((char *)bufPtr).substring(0,4);
    bufPtr += 4;
    uint32_t chunkSize = getUInt32();
    Serial.print(F("\nfound chunk type ")); Serial.print(chunkType);
    Serial.print(F(" with size ")); Serial.print(chunkSize);
    Serial.print(F(" at ")); Serial.print((uint32_t)(bufPtr - buffer));
    Serial.print(F(" of ")); Serial.println((uint32_t)(eof-buffer));

    if (chunkType == "fmt ") {
      if (getUInt16() != 1) fatal(F("not uncompressed Microsoft format"));
      channels       = getUInt16();
      framesPerSec   = getUInt32(); 
      avgBytesPerSec = getUInt32();
      frameSize      = getUInt16();
      bitsPerSample  = getUInt16();
      Serial.print(F("channels: "));       Serial.println(channels);
      Serial.print(F("framesPerSec: "));   Serial.println(framesPerSec);
      Serial.print(F("avgBytesPerSec: ")); Serial.println(avgBytesPerSec);
      Serial.print(F("frameSize: "));      Serial.println(frameSize);
      Serial.print(F("bitsPerSample: "));  Serial.println(bitsPerSample);
    }
    
    else if (chunkType == "fact") {
      numSamples = getUInt32(); 
      Serial.print(F("numSamples: ")); Serial.println(numSamples);
    }
    else if (chunkType == "data") {
      while(1);
    }
    else fatal(F("invalid chunk type "));
  }
  
  // Serial.println(F("Playing"));
  // while (myFile.available()) {
  //   myFile.read(buffer, sizeof(buffer));
  //   uint8_t i;
  //   for (i=0; i < sizeof(buffer); i++) {
  //     dac.setVoltage(buffer[i], false);
  //   } 
  // }
  // Serial.println(F("Done playing\r\n"));
}

void loop(void) {
}
