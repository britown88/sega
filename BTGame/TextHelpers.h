#pragma once

#include "segautils/StandardVectors.h"
#include "segautils/Defs.h"

//generate a single-byte that contains the palette indices for a fg and bg
//precede this with \c in a textline to change color on the fly
typedef unsigned char byte;

byte textGenerateColorCode(byte bg, byte fg);
void textExtractColorCode(byte c, byte *bg, byte *fg);

 void stringRenderToArea(const char *str, size_t lineWidth, vec(StringPtr) *outList);

