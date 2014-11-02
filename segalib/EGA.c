#include "EGA.h"
#include "BitBuffer.h"
#include "CheckedMemory.h"
#include "BitTwiddling.h"

#include <malloc.h>
#include <string.h>
#include <stdio.h>

Frame *frameCreate() {
   return checkedCalloc(1, sizeof(Frame));
}
void frameDestroy(Frame *self) {
   checkedFree(self);
}

void _scanLineRenderImageScanLine(ScanLine *sl, short position, byte *colorBuffer, byte *alphaBuffer, short bitOffset, short width) {
   int i;
   for(i = 0; i < width; ++i) {
      byte t = scanLineGetBit((ScanLine*)alphaBuffer, bitOffset + i);//trans
      byte c = scanLineGetBit((ScanLine*)colorBuffer, bitOffset + i);//c
      byte s = scanLineGetBit(sl, position + i);//screen

      //if alpha, use color, else use screen
      scanLineSetBit(sl, position + i, (t&c) + ((~t)&s));
   }

}

ImageScanLine *createSmallestScanLine(short bitCount, byte *data){
   ImageScanLine *scanline = 0;
   scanline = createSolidScanLine(bitCount, data);
   if(!scanline) {
      scanline = createRLEScanLine(bitCount, data);
      if(!scanline) {
         scanline = createUncompressedScanLine(bitCount, data);
      }
   }

   return scanline;
}

void frameRenderImage(Frame *self, short x, short y, Image *img) {
   short imgWidth = imageGetWidth(img);
   short imgHeight = imageGetHeight(img);

   short clipSizeX, clipSizeY, ignoreOffsetX, ignoreOffsetY;
   int j, i;
   byte colorBuffer[MAX_IMAGE_WIDTH];
   byte alphaBuffer[MAX_IMAGE_WIDTH];

   ignoreOffsetX = x < 0 ? -x : 0;
   ignoreOffsetY = y < 0 ? -y : 0;

   clipSizeX = imgWidth;
   clipSizeY = imgHeight;
   if(clipSizeX + x > EGA_RES_WIDTH) clipSizeX = EGA_RES_WIDTH - x;
   if(clipSizeY + y > EGA_RES_HEIGHT) clipSizeY = EGA_RES_HEIGHT - y;

   clipSizeX -= ignoreOffsetX;
   clipSizeY -= ignoreOffsetY;

   if(clipSizeX <= 0 || clipSizeY <= 0) {
      return;
   }

   for(j = 0; j < clipSizeY; ++j) {
      imageScanLineRender(imageGetScanLine(img, j + ignoreOffsetY, 0), alphaBuffer);//transparency

      for(i = 0; i < EGA_PLANES; ++i) {
         imageScanLineRender(imageGetScanLine(img, j + ignoreOffsetY, i + 1), colorBuffer);

         _scanLineRenderImageScanLine(&self->planes[i].lines[j+y+ignoreOffsetY], x + ignoreOffsetX, 
            colorBuffer, alphaBuffer, ignoreOffsetX, clipSizeX);
      }
   }
}

byte scanLineGetBit(ScanLine *self, short position) {
   return  getBitFromArray(self->pixels, position);
}

void scanLineSetBit(ScanLine *self, short position, byte value) {
   setBitInArray(self->pixels, position, value);
}

void buildEGAColorTable(int *table) {
   int i;
   //                  00 01  10   11
   byte rgbLookup[] = {0, 85, 170, 255};
   for(i = 0; i < EGA_COLORS; ++i) {
      byte shift = 5;

      byte r = getBit(i, shift--);
      byte g = getBit(i, shift--);
      byte b = getBit(i, shift--);
      byte R = getBit(i, shift--);
      byte G = getBit(i, shift--);
      byte B = getBit(i, shift);

      byte rgb_r = rgbLookup[(R << 1) + r];
      byte rgb_g = rgbLookup[(G << 1) + g];
      byte rgb_b = rgbLookup[(B << 1) + b];

      byte out[] = {rgb_r, rgb_g, rgb_b, 255};

      table[i] = *(int*)out;
   }
}

int getEGAColor(byte c) {
   static int lookup[EGA_COLORS] = {0};
   static int loaded = 0;

   if(!loaded) {
      buildEGAColorTable(lookup);
      loaded = 1;
   }

   return lookup[c];

}

Palette paletteDeserialize(const char *path) {
   long fSize;
   BitBuffer *buffer = bitBufferCreate(readFullFile(path, &fSize), 1);
   Palette p = {0};

   int i;

   if(fSize == 12) {
      for(i = 0; i < 16; ++i) {
         bitBufferReadBits(buffer, (byte*)&p+i, 6);
      }
   }

   return p;
}

void paletteSerialize(byte *data, const char *path) {
   BitBuffer *buff = bitBufferCreate(checkedCalloc(1, 12), 1);
   int i;
   FILE *out;
   for(i = 0; i < 16; ++i) {
      bitBufferWriteBits(buff, 6, data + i);
   }

   out = fopen(path, "wb");
   fwrite(bitBufferGetData(buff), sizeof(char), 12, out);
   fclose (out);
}

Frame *buildCheckerboardFrame(int width, byte color1, byte color2) {
   Frame *fb = frameCreate();
   int x, y, j;

   for(y = 0; y < EGA_RES_HEIGHT; ++y){
      for(x = 0; x < EGA_RES_WIDTH;++x) {

         byte currentColor =  ((x / width) + (y/width)) % 2 ? color1 : color2;

         for(j = 0; j < EGA_PLANES; ++j) {
            scanLineSetBit(&fb->planes[j].lines[y], x, !!(currentColor&(1<<j)));
         }
      }
   }

   return fb;
}

