#include "Conways.h"
#include "segashared/CheckedMemory.h"
#include "segalib/EGA.h"
#include "segautils/Defs.h"
#include "SEGA/App.h"

static byte _colorAt(Texture *tex, short x, short y) {
   byte color = 0;

   int i;
   for (i = 0; i < EGA_PLANES; ++i) {
      setBit(&color, i, getBitFromArray(textureGetScanline(tex, i, y), x));
   }
   return color;
}

static int _neighborIs(Texture *tex, short x, short y, byte color) {
   return _colorAt(tex, x, y) == color ? 1 : 0;
}

static int _getNeightbors(Texture *tex, FrameRegion *region, short x, short y, byte color) {
   int neighborCount = 0;

   bool yUp = y > region->origin_y;
   bool yDown = y < region->origin_y + region->height - 2;
   bool xLeft = x > region->origin_x;
   bool xRight = x < region->origin_x + region->width - 2;

   if (xLeft) {
      if (yUp) {
         neighborCount += _neighborIs(tex, x - 1, y - 1, color);
      }

      neighborCount += _neighborIs(tex, x - 1, y, color);

      if (yDown) {
         neighborCount += _neighborIs(tex, x - 1, y + 1, color);
      }
   }

   if (yUp) {
      neighborCount += _neighborIs(tex, x, y - 1, color);
   }

   if (yDown) {
      neighborCount += _neighborIs(tex, x, y + 1, color);
   }

   if (xRight) {
      if (yUp) {
         neighborCount += _neighborIs(tex, x + 1, y - 1, color);
      }

      neighborCount += _neighborIs(tex, x + 1, y, color);

      if (yDown) {
         neighborCount += _neighborIs(tex, x + 1, y + 1, color);
      }
   }

   return neighborCount;
}


//returns color
static byte _checkReproduction(Texture *tex, FrameRegion *region, short x, short y) {

   byte colorCounts[EGA_PALETTE_COLORS] = { 0 };
   int i = 0;
   byte foundColor = 0;

   bool yUp = y > region->origin_y;
   bool yDown = y < region->origin_y + region->height - 2;
   bool xLeft = x > region->origin_x;
   bool xRight = x < region->origin_x + region->width - 2;


   if (xLeft) {
      if (yUp) {
         ++colorCounts[_colorAt(tex, x-1, y-1)];
      }

      ++colorCounts[_colorAt(tex, x-1, y)];

      if (yDown) {
         ++colorCounts[_colorAt(tex, x-1, y+1)];
      }
   }

   if (yUp) {
      ++colorCounts[_colorAt(tex, x, y-1)];
   }

   if (yDown) {
      ++colorCounts[_colorAt(tex, x, y+1)];
   }

   if (xRight) {
      if (yUp) {
         ++colorCounts[_colorAt(tex, x+1, y-1)];
      }

      ++colorCounts[_colorAt(tex, x+1, y)];

      if (yDown) {
         ++colorCounts[_colorAt(tex, x+1, y+1)];
      }
   }

   for (i = 1; i < EGA_PALETTE_COLORS; ++i) {
      if (colorCounts[i] == 3) {
         if (foundColor != 0) {
            foundColor = appRand(appGet(), 0, 2) ? foundColor : i;
         }
         else {
            foundColor = i;
         }
      }
   }

   return foundColor;
}

//returns color
static byte _isAlive(Texture *tex, FrameRegion *region, short x, short y) {
   byte color = _colorAt(tex, x, y);
   int neighbors = 0;

   if (!color) {
      return _checkReproduction(tex, region, x, y);
   }

   neighbors = _getNeightbors(tex, region, x, y, color);
   return (neighbors < 2 || neighbors > 3) ? 0 : color;
}




void conwaysRender(Texture *tex, FrameRegion *region) {
   Texture *newFrame = textureCreate(textureGetWidth(tex), textureGetHeight(tex));
   short x, y;

   for (y = region->origin_y; y < region->origin_y + region->height; ++y) {
      for (x = region->origin_x; x < region->origin_x + region->width; ++x) {    
         bool color = _isAlive(tex, region, x, y);
         textureRenderPoint(newFrame, region, x - region->origin_x, y - region->origin_y, color);
      }
   }

   textureRenderTexture(tex, region, 0, 0, newFrame);
   textureDestroy(newFrame);
}