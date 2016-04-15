
#include "PetitFS.h"
#include "PetitSerial.h"
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <MemoryFree.h>

////////////////////////// MISC DEF ///////////////////////////
PetitSerial PS;
#define Serial PS
FATFS fs;

// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

void dbg(__FlashStringHelper* msg, uint32_t value) {
  Serial.print(msg); Serial.println(value);
}
  
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


// ////////////////////////// SD FILE READ VARS ///////////////////////////
#define BUFLEN 512

uint8_t buffer[2*BUFLEN];
#define buffer1 (buffer)
#define buffer2 (buffer + BUFLEN)

uint8_t *bufPtr;
char fileName[] = "TEST.WAV";

// shared with TWI interrupts
volatile uint8_t *bufWritePtr = buffer1;
volatile uint8_t *bufReadPtr  = buffer1;

uint32_t fileLength;
uint16_t channels;
uint32_t framesPerSec;
uint32_t avgBytesPerSec;
uint16_t frameSize;
uint16_t bitsPerSample;
uint32_t numSamples;

uint32_t bytesAddedToBuf = 0;
uint32_t bytesRemovedFromBuf = 0;
uint32_t totalBytesRead = 0;


////////////////////// BUFFER ACCESS UTILS //////////////////////
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
  dbg(F("freeMemory: "), freeMemory());
  if (pf_mount(&fs)) fatal(F("initial mount"));
}


////////////////////// LOOP //////////////////////
// one file (song) per loop

void loop(void) {
file_loop:
  uint16_t bytesRead;
  bool bufEmpty, readingBuf1, bufReadDone = false;
  dbg(F("new file, freeMemory: "), freeMemory());
  Serial.print(F("opening file: ")); Serial.println(fileName);
  if (pf_open(fileName)) fatal(F("fs open"));

  // let audio sample reading old file finish
  while (!bufReadDone) {
    noInterrupts();
    bufReadDone = (bufReadPtr == bufWritePtr);
    interrupts();
  }
  
  if (pf_read(buffer1, BUFLEN, &bytesRead)) fatal(F("file init-read"));
  dbg(F("init bytesRead: "), bytesRead);
  
  totalBytesRead = bytesRead;
  noInterrupts();
  bufWritePtr = buffer1;
  bufReadPtr  = buffer1;
  interrupts();

  bufPtr = buffer1;
  if (!isEqStr("RIFF", 4)) fatal(F("not .wav file"));
  fileLength = getUInt32() + 8;
  if (!isEqStr("WAVE", 4)) fatal(F("no WAVE"));
  
  // chunk_loop:
  while (true) {
    String chunkType = String((char *)bufPtr).substring(0,4);
    bufPtr += 4;
    uint32_t chunkSize = getUInt32();
    Serial.print(F("\nfound chunk type ")); Serial.print(chunkType);
    Serial.print(F(" with size ")); Serial.print(chunkSize);
    Serial.print(F(" at ")); Serial.print((uint32_t)(bufPtr - buffer));
    Serial.print(F(" of ")); Serial.println(fileLength);

    if (chunkType == "fmt ") {
      if (getUInt16() != 1) fatal(F("not uncompressed Microsoft format"));
      channels       = getUInt16();
      framesPerSec   = getUInt32(); 
      avgBytesPerSec = getUInt32();
      frameSize      = getUInt16();
      bitsPerSample  = getUInt16();
      dbg(F("channels:       "), channels);
      dbg(F("framesPerSec:   "), framesPerSec);
      dbg(F("avgBytesPerSec: "), avgBytesPerSec);
      dbg(F("frameSize:      "), frameSize);
      dbg(F("bitsPerSample:  "), bitsPerSample);
    }
    else if (chunkType == "fact") {
      numSamples = getUInt32(); 
      dbg(F("numSamples:      "), numSamples);
    }  
    // assumes one data chunk per file
    else if (chunkType == "data") {
      noInterrupts();
      bufWritePtr = buffer1;
      bufReadPtr  = buffer1;
      interrupts();
      dbg(F("new file, freeMemory: "), freeMemory());
      
      // read one SD data block per loop
      while(true) {
        uint32_t waited = 0;
        do {
          waited++;
          if (waited == 2) Serial.print(F("waiting ... "));
          noInterrupts();
          bufEmpty = (bytesRemovedFromBuf == bytesAddedToBuf);
          readingBuf1 = bufReadPtr < buffer2;
          interrupts();
        } while (!bufEmpty &&
                ((bufWritePtr == buffer1 &&  readingBuf1) ||
                 (bufWritePtr == buffer2 && !readingBuf1)));
        if (waited > 1) Serial.println();
        
        if (pf_read((void*)bufWritePtr, BUFLEN, &bytesRead)) fatal(F("data read"));
        dbg(F("data bytesRead: "), bytesRead);
        totalBytesRead += bytesRead;
        
        noInterrupts();
        bytesAddedToBuf += bytesRead;
        bufWritePtr += bytesRead;
        if (bufWritePtr == buffer2+BUFLEN) bufWritePtr = buffer1;
        interrupts();
        
        if (bytesRead < BUFLEN || totalBytesRead >= fileLength) {
          dumpHex(buffer, 2*BUFLEN);
          dbg(F("bytesRead: "), bytesRead);
          dbg(F("fileLength: "), fileLength);
          dbg(F("done reading file, totalBytesRead: "), totalBytesRead);
          // pf_close(); doesn't exist?
          while(1);
          goto file_loop;
        }
      }
    }
    else fatal(F("invalid chunk type "));
  }
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
