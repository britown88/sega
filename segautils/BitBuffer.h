#pragma once

typedef unsigned char byte;

typedef struct BitBuffer_t BitBuffer;

BitBuffer *bitBufferCreate(byte *existingData, int deleteData);
void bitBufferDestroy(BitBuffer *self);

void bitBufferReadBits(BitBuffer *self, byte *destination, int bitCount);
short bitBufferReadShort(BitBuffer *self);

void bitBufferWriteBits(BitBuffer *self, int bitCount, byte *data);

byte *bitBufferGetData(BitBuffer *self);

long bitBufferGetPosition(BitBuffer *self);

byte *readFullFile(const char *path, long *fsize);