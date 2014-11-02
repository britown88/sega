#pragma once

#include "ImageTools.hpp"
#include "EGA.h"

#ifdef __cplusplus
extern "C" {
#endif
float colorDistance(Color c1, Color c2);


#pragma region OLL_SHIT
struct PaletteColor;
 
struct PaletteEntry
{
   PaletteEntry *next, *prev;
   float distance;
   PaletteColor* color;
};
 
typedef PaletteEntry* pPaletteEntry;
 
struct PaletteColor
{
   byte removable;
   byte pPos;
   byte EGAColor;
   PaletteEntry entries[64];
   PaletteColor():removable(1){}
   PaletteColor(byte c):EGAColor(c), removable(1){}
   PaletteColor(byte c, byte targetPalettPosition):EGAColor(c), removable(0), pPos(targetPalettPosition){}
};
 
struct ImageColor
{
   pPaletteEntry closestColor;
   ImageColor():closestColor(0){}
};

struct rgbega{
   int rgb;
   byte ega;
   rgbega(){}
   rgbega(int _rgb, byte _ega):rgb(_rgb), ega(_ega){}
   bool operator<(int other) {
      return rgb < other;
   }
};

Color EGAColorLookup(byte c);
byte closestEGA(int rgb);
void insertSortedPaletteEntry(ImageColor &color, PaletteColor &parent, byte target, byte current, PaletteEntry &out);
bool isClosest(ImageColor &color, PaletteEntry &entry);

#pragma endregion

#ifdef __cplusplus
};
#endif