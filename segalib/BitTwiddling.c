#include "BitTwiddling.h"
#include "BitBuffer.h"
#include "CheckedMemory.h"

#include <string.h>

int minByteCount(int bitCount) {
   return bitCount/8 + !!(bitCount%8);
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
      dest[pos/8] |= 1 << pos%8;
   }
   else {
      dest[pos/8] &= ~(1 << pos%8);
   }
}

byte getBit(byte dest, byte pos/*0-7*/){
   return !!(dest & (1<<(pos%8)));
}

byte getBitFromArray(const byte *dest, int pos){
   return !!(dest[pos/8] & (1<<(pos%8)));
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
   byte lineOutput[MAX_IMAGE_WIDTH] = {0};

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
