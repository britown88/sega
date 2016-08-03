#include "BitBuffer.h"
#include "segashared\CheckedMemory.h"
#include "BitTwiddling.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>



BitBuffer bitBufferCreate(byte *existingData, int deleteData) {
   BitBuffer out = { .buffer = existingData,  .deleteData = deleteData, .pos = 0 };
   return out;
}
void bitBufferDestroy(BitBuffer *self) {
   if(self->deleteData) {
      checkedFree(self->buffer);
   }
}

void bitBufferReadBits(BitBuffer *self, byte *destination, int bitCount) {
   int i;
   for(i = 0; i < bitCount; ++i) {
      long position = self->pos + i;
      byte value = getBitFromArray(self->buffer, position);
      setBitInArray(destination, i, value);
   }

   self->pos += bitCount;
}
short bitBufferReadShort(BitBuffer *self) {
   static byte readBuffer[sizeof(uint16_t)] = {0};
   bitBufferReadBits(self, readBuffer, sizeof(uint16_t)*8);
   return *(short*)readBuffer;
}

void bitBufferWriteBits(BitBuffer *self, int bitCount, byte *data) {
   int i;
   for(i = 0; i < bitCount; ++i) {
      int position = self->pos + i;

      byte value = getBitFromArray(data, i);
      setBitInArray(self->buffer, position, value);
   }

   self->pos += bitCount;
}

byte *readFullFile(const char *path, long *fsize) {
   byte *string;
   FILE *f = fopen(path, "rb");

   if (!f) {
      return NULL;
   }

   fseek(f, 0, SEEK_END);
   *fsize = ftell(f);
   fseek(f, 0, SEEK_SET);

   string = checkedMalloc(*fsize + 1);
   fread(string, *fsize, 1, f);
   fclose(f);

   string[*fsize] = 0;

   return string;
}
