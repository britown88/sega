#pragma once

typedef unsigned char byte;

typedef struct BitBuffer_t{
   long pos;
   int deleteData;
   byte *buffer;
} BitBuffer;

BitBuffer bitBufferCreate(byte *existingData, int deleteData);
void bitBufferDestroy(BitBuffer *self);

void bitBufferReadBits(BitBuffer *self, byte *destination, int bitCount);
short bitBufferReadShort(BitBuffer *self);

void bitBufferWriteBits(BitBuffer *self, int bitCount, byte *data);

byte *readFullFile(const char *path, long *fsize);