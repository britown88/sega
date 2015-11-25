#include "BitTwiddling.h"
#include "BitBuffer.h"
#include "segashared\CheckedMemory.h"

#include <string.h>
#include <stdint.h>


#define MAX_BUFFER_WIDTH 1024

// '0-9 A-F' from 0-15 value
byte asciiFrom4BitHex(byte b) {
   b &= 15;
   return b > 9 ? (b - 10 + 'A') : b;
}

size_t hashPtr(void* ptr){
   size_t out = 5031;
   int i;
   for (i = 0; i < sizeof(void*); ++i) {
      out += (out << 5) + ((char*)&ptr)[i];
   }
   return out;
}

int minByteCount(int bitCount) {
   return (bitCount >> 3) + !!(bitCount & 7);
}

int minIntCount(int bitCount) {
   return bitCount / (sizeof(int32_t) * 8) + !!(bitCount % (sizeof(int32_t) * 8));
}

void setBit(byte *dest, byte pos/*0-7*/, byte value/*0-1*/){
   if(value) {
      *dest |= 1 << pos;
   }
   else {
      *dest &= ~(1 << pos);
   }
}

void setBitInArray(byte *dest, int pos, byte value/*0-1*/) {
   if(value) {
      dest[pos >> 3] |= 1 << (pos & 7);
   }
   else {
      dest[pos >> 3] &= ~(1 << (pos & 7));
   }
}

byte getBit(byte dest, byte pos/*0-7*/){
   return !!(dest & (1<< (pos & 7)));
}

byte getBitFromArray(const byte *dest, int pos){
   //return !!(dest[pos/8] & (1<<(pos%8)));
   return !!(dest[pos >> 3] & (1 << (pos & 7)));//i assume release is making this optimization for us?
}



//returns 0 if passed the bitcount(failure), 1 on success
int _RLEWriteCurrent(BitBuffer *buffer, int maxBitCount, byte current, byte currentCount){
   if(bitBufferGetPosition(buffer) + 9 > maxBitCount){
      bitBufferDestroy(buffer);
      return 0;
   }

   bitBufferWriteBits(buffer, 8, &currentCount);
   bitBufferWriteBits(buffer, 1, &current);
   return 1;

}

//returns 0 if compressed size is larger than inBitCount
int compressBitsRLE(const byte *in, const int inBitCount, byte *out) {
   
   int i, ret;
   byte current = getBit(*in, 0), currentCount = 1;
   BitBuffer *buffer = bitBufferCreate(out, 0);

   for(i = 1; i < inBitCount; ++i){
      byte value = getBitFromArray(in, i);

      if(value == current) {
         if(currentCount == 255){

            if(!_RLEWriteCurrent(buffer, inBitCount, current, currentCount))
               return 0;

            currentCount = 0;
         }

         ++currentCount;
      }
      else {
         if(!_RLEWriteCurrent(buffer, inBitCount, current, currentCount))
            return 0;

         currentCount = 1;
         current = value;
      }
   }

   if(currentCount > 0) {
      if(!_RLEWriteCurrent(buffer, inBitCount, current, currentCount))
         return 0;
   }
   
   ret = bitBufferGetPosition(buffer);
   bitBufferDestroy(buffer);
   return ret;
}

void decompressRLE(byte *src, int compressedBitCount, byte *dest){
   BitBuffer *srcBuff = bitBufferCreate(src, 0);
   BitBuffer *destBuff = bitBufferCreate(dest, 0);
   byte lineOutput[MAX_BUFFER_WIDTH] = { 0 };

   while(bitBufferGetPosition(srcBuff) + 9 <= compressedBitCount) {
      byte count = 0, value = 0;
      bitBufferReadBits(srcBuff, &count, 8);
      bitBufferReadBits(srcBuff, &value, 1);

      if(value > 0) value = 255;

      memset(lineOutput, value, minByteCount(count));
      bitBufferWriteBits(destBuff, count, lineOutput);      
   }

   bitBufferDestroy(srcBuff);
   bitBufferDestroy(destBuff);
}

byte arrayIsSolid(byte *src, int bitCount){
   byte value = getBitFromArray(src, --bitCount);
   while(bitCount){
      if(value != getBitFromArray(src, --bitCount))
         return 0;
   }
   return 1;
}

#ifdef __GNUC__
   #include <stdlib.h>
   unsigned long BSR32(unsigned long value){
      return __builtin_clzl(value);
   } 
   void STOSD(unsigned long *dest, unsigned long val, size_t count){
      //TODO: figure out how to do this the ASM way
      size_t i;
      for(i = 0; i < count; ++i){
         dest[i] = val;
      }
   }
#elif _MSC_VER
   #include <intrin.h>
   #pragma intrinsic(_BitScanReverse)
   unsigned long BSR32(unsigned long value){
      unsigned long out = 0;
      _BitScanReverse(&out, value);
      return out;
   }
   #pragma intrinsic(__stosd)
   void STOSD(unsigned long *dest, unsigned long val, size_t count){
      __stosd(dest, val, count);
   }
#endif


