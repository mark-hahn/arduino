
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
#define BUFLEN 64
    
void fatal(String msg) {
  Serial.print(String("Fatal error: " + msg));
  while(1);
}

void dumpHex(uint8_t* buf, int len) {
  int cnt = 0;
  uint8_t* max = buf + len;
  while(buf < max) {
    uint8_t* max2 = buf + 16;
    for(; buf < max2 && cnt < len; buf++, cnt++) {
      String hd = String(*buf, HEX); 
      if (hd.length() < 2) hd = "0" + hd;
      Serial.print(hd); Serial.print(" ");
      if (cnt % 4 == 3) Serial.print(" ");
    }
    Serial.println();
  }
}

////////////////////////// DAC DEF ///////////////////////////

Adafruit_MCP4725 dac;

////////////////////////// SD FILE READ ///////////////////////////

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

File myFile;
uint32_t totalBytesRead = 0;
bool fileReadActive = false;

void readFile() {
  Serial.println("readFile");
  
  // if (!fileReadActive) return;
  
  // noInterrupts();
  bool bufEmpty = (bufWritePtr == bufReadPtr);
  // interrupts();
  
  Serial.print("bufEmpty: "); Serial.println(bufEmpty);
  Serial.print("bufWritePtr: "); Serial.println((uint32_t)bufWritePtr);
  
  while (!bufEmpty &&
         ((bufWritePtr == buffer1 && bufReadPtr  < buffer2) ||
          (bufWritePtr == buffer2 && bufReadPtr >= buffer2))) {}
  
  Serial.print("buffer: "); Serial.println((uint32_t)buffer);
  Serial.print("buffer1: "); Serial.println((uint32_t)buffer1);
  Serial.print("buffer2: "); Serial.println((uint32_t)buffer2);
  
  uint32_t bytesRead = myFile.read((void*)bufWritePtr, BUFLEN);
  
  Serial.print("bytesRead: "); Serial.println(bytesRead);
  totalBytesRead += bytesRead;
  Serial.print("totalBytesRead: "); Serial.println(totalBytesRead);
  
  dumpHex(buffer, 64);
  
  // noInterrupts();
  bufWritePtr += bytesRead;
  if (bufWritePtr >= buffer2+BUFLEN) bufWritePtr = buffer1;
  // interrupts();
  
  if (bytesRead < BUFLEN ||
       (fileLength < 1000000000 && 
        totalBytesRead >= fileLength)) {
    fileReadActive = false;
    Serial.print("done reading file, bytes read: "); 
    Serial.println(totalBytesRead);
  }
}

void openFile(const char* fileName) {
  Serial.print("opening file: "); Serial.println(fileName);
  pinMode(4, OUTPUT);
  if (!SD.begin(4)) fatal("SD initialization failed");
  
  myFile = SD.open(fileName, FILE_READ);
  if (!myFile) fatal("error opening TEST.WAV");
  fileReadActive = true;
  
  // noInterrupts();
  bufWritePtr = buffer1;
  bufReadPtr  = buffer1;
  // interrupts();
  
  readFile();
}

////// BUFFER ACCESS FUNCTIONS /////

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

void setup() {
  Serial.begin(115200);
  
  openFile("TEST.WAV");
  
  if (!isEqStr("RIFF", 4)) fatal("not .wav file");
  fileLength = getUInt32() + 8;
  if (!isEqStr("WAVE", 4)) fatal("no WAVE");

  uint8_t *eof = bufPtr + fileLength;
  
  // Serial.print("chunkType: "); Serial.println(chunkType);
  // chunk loop
  while (bufPtr < eof) {
    String chunkType = String((char *)bufPtr).substring(0,4);
    bufPtr += 4;
    uint32_t chunkSize = getUInt32();
    Serial.print("\nfound chunk type " + chunkType + 
                 " with size " + chunkSize +
                 " at " + (uint32_t)(bufPtr - buffer) +
                 " of " + (uint32_t)(eof-buffer) + "\r\n");    
    if (chunkType == "fmt ") {
      if (getUInt16() != 1) fatal("not uncompressed Microsoft format");
      channels       = getUInt16();
      framesPerSec   = getUInt32(); 
      avgBytesPerSec = getUInt32();
      frameSize      = getUInt16();
      bitsPerSample  = getUInt16();
      Serial.print("channels: ");       Serial.println(channels);
      Serial.print("framesPerSec: ");   Serial.println(framesPerSec);
      Serial.print("avgBytesPerSec: "); Serial.println(avgBytesPerSec);
      Serial.print("frameSize: ");      Serial.println(frameSize);
      Serial.print("bitsPerSample: ");  Serial.println(bitsPerSample);
    }
    
    else if (chunkType == "fact") {
      numSamples = getUInt32(); 
      Serial.print("numSamples: "); Serial.println(numSamples);
    }
    else if (chunkType == "data") {
      while(1);
    }
    else fatal(String("invalid chunk type " + chunkType));
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
