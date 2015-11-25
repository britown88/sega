#pragma once

#include "extern_c.h"
#include <stddef.h>

SEXTERN_C

typedef unsigned char byte;


size_t hashPtr(void* ptr);

int minByteCount(int bitCount);
int minIntCount(int bitCount);
void setBit(byte *dest, byte pos/*0-7*/, byte value/*0-1*/);
void setBitInArray(byte *dest, int pos, byte value/*0-1*/);
byte getBit(byte dest, byte pos/*0-7*/);
byte getBitFromArray(const byte *dest, int pos);

byte asciiFrom4BitHex(byte b);

//returns 0 if compressed size is larger than inBitCount
int compressBitsRLE(const byte *in, const int inBitCount, byte *out);
void decompressRLE(byte *src, int compressedBitCount, byte *dest);

byte arrayIsSolid(byte *src, int bitCount);

unsigned long BSR32(unsigned long value);
void STOSD(unsigned long *dest, unsigned long val, size_t count);

END_SEXTERN_C