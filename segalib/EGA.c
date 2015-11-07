#include "EGA.h"
#include "segautils\BitBuffer.h"
#include "segashared\CheckedMemory.h"
#include "segautils\BitTwiddling.h"
#include "segautils\Defs.h"

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

Frame *frameCreate() {
   return checkedCalloc(1, sizeof(Frame));
}
void frameDestroy(Frame *self) {
   checkedFree(self);
}

void _scanLineRenderBit(ScanLine *sl, byte *colorBuffer, byte *alphaBuffer, int *imgX, int *x){
   //setbits...
   byte t = scanLineGetBit((ScanLine*)alphaBuffer, *imgX);//trans
   byte c = scanLineGetBit((ScanLine*)colorBuffer, *imgX);//c
   byte s = scanLineGetBit(sl, *x);//screen

   //if alpha, use color, else use screen
   scanLineSetBit(sl, *x, ((s & t) | c));

   ++(*x);
   ++(*imgX);
}

void _scanLineRender8Bits(ScanLine *sl, byte *colorBuffer, byte *alphaBuffer, int *imgX, int *x, int byteRun){
   int i;
   int frac = (*imgX) % 8;  //how far we are into the image 

   //1 << 3 == 8
   uint8_t *screenArr = ((uint8_t*)sl->pixels) + ((*x) >> 3);
   uint8_t *imgArr = ((uint8_t*)colorBuffer) + ((*imgX) >> 3);
   uint8_t *alphaArr = ((uint8_t*)alphaBuffer) + ((*imgX) >> 3);

   if (!frac) { //fast path!
      for (i = 0; i < byteRun; ++i, (*x) += 8, (*imgX) += 8) {
         //simply aligned, do regular int operations
         *screenArr &= *alphaArr++;
         *screenArr++ |= *imgArr++;
      }
   }
   else {//slow path...
      for (i = 0; i < byteRun; ++i, (*x) += 8, (*imgX) += 8) {
         //since this is unaligned, we do it in two slices.  Let's build the image bits into an aligned int32_t.
         *screenArr &= (*alphaArr >> frac) | ((*(alphaArr + 1)) << (8 - frac));
         *screenArr |= (*imgArr >> frac) | ((*(imgArr + 1)) << (8 - frac));

         ++imgArr;
         ++screenArr;
         ++alphaArr;
      }
   }

}

void _scanLineRender32Bits(ScanLine *sl, byte *colorBuffer, byte *alphaBuffer, int *imgX, int *x, int intRun){
   int i;
   int intBits = sizeof(uint32_t) * 8;
   int frac = *imgX % intBits;  //how far we are into the image 
   
   //1 << 5 == 32
   uint32_t *screenArr = ((uint32_t*)sl->pixels) + (*x >> 5);
   uint32_t *imgArr = ((uint32_t*)colorBuffer) + (*imgX >> 5);
   uint32_t *alphaArr = ((uint32_t*)alphaBuffer) + (*imgX >> 5);   

   if (!frac) { //fast path!
      for (i = 0; i < intRun; ++i, *x += intBits, *imgX += intBits) {
         //simply aligned, do regular int operations
         *screenArr &= *alphaArr++;
         *screenArr++ |= *imgArr++;
      }
   }
   else {//slow path...
      for (i = 0; i < intRun; ++i, *x += intBits, *imgX += intBits) {
         //since this is unaligned, we do it in two slices.  Let's build the image bits into an aligned int32_t.
         *screenArr &= (*alphaArr >> frac) | ((*(alphaArr + 1)) << (intBits - frac));
         *screenArr |= (*imgArr >> frac) | ((*(imgArr + 1)) << (intBits - frac));

         ++imgArr;
         ++screenArr;
         ++alphaArr;
      }
   }
}

void _scanLineRenderImageScanLine(ScanLine *sl, short screenPos, byte *colorBuffer, byte *alphaBuffer, short bitOffset, short width) {
   int intRun, byteRun, alignRun;
   int imgX = bitOffset;
   int x = screenPos;
   int last = x + width;

   int intBits = sizeof(uint32_t) * 8;
   int alignedBits = x % 8;

   if (alignedBits){
      while (x < last && alignedBits++ < 8) {
         _scanLineRenderBit(sl, colorBuffer, alphaBuffer, &imgX, &x);
      }
   }

   if (x == last)  {
      return;
   }

   byteRun = (last - x ) / 8;
   alignRun = (4 - ((x % intBits) / 8)) % 4;
   _scanLineRender8Bits(sl, colorBuffer, alphaBuffer, &imgX, &x, MIN(alignRun, byteRun));
   
   intRun = (last - x) / intBits;
   _scanLineRender32Bits(sl, colorBuffer, alphaBuffer, &imgX, &x, intRun);   

   byteRun = (last - x) / 8;
   _scanLineRender8Bits(sl, colorBuffer, alphaBuffer, &imgX, &x, byteRun);

   while (x < last) {
      _scanLineRenderBit(sl, colorBuffer, alphaBuffer, &imgX, &x);
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

void frameRenderImagePartial(Frame *self, FrameRegion *vp, short x, short y, Image *img, short imgX, short imgY, short subimgWidth, short subimgHeight) {
   short imgWidth = MIN(subimgWidth, imageGetWidth(img));
   short imgHeight = MIN(subimgHeight, imageGetHeight(img));

   short clipSizeX, clipSizeY, ignoreOffsetX, ignoreOffsetY;
   short borderRight, borderBottom;
   int j, i;
   byte colorBuffer[MAX_IMAGE_WIDTH];
   byte alphaBuffer[MAX_IMAGE_WIDTH];

   ignoreOffsetX = x < 0 ? -x : 0;
   ignoreOffsetY = y < 0 ? -y : 0;

   x += vp->origin_x;
   y += vp->origin_y;

   borderRight = MIN(EGA_RES_WIDTH, vp->origin_x + vp->width);
   borderBottom = MIN(EGA_RES_HEIGHT, vp->origin_y + vp->height);

   clipSizeX = imgWidth;
   clipSizeY = imgHeight;
   if (clipSizeX + x > borderRight) clipSizeX = borderRight - x;
   if (clipSizeY + y > borderBottom) clipSizeY = borderBottom - y;

   clipSizeX -= ignoreOffsetX;
   clipSizeY -= ignoreOffsetY;

   if (clipSizeX <= 0 || clipSizeY <= 0) {
      return;
   }

   for (j = 0; j < clipSizeY && j + ignoreOffsetY < imgHeight; ++j) {
      imageScanLineRender(imageGetScanLine(img, j + imgY + ignoreOffsetY, 0), alphaBuffer);//transparency

      for (i = 0; i < EGA_PLANES; ++i) {
         imageScanLineRender(imageGetScanLine(img, j + imgY + ignoreOffsetY, i + 1), colorBuffer);

         _scanLineRenderImageScanLine(&self->planes[i].lines[j + y + ignoreOffsetY], x + ignoreOffsetX,
            colorBuffer, alphaBuffer, imgX + ignoreOffsetX, clipSizeX);
      }
   }
}

void frameRenderImage(Frame *self, FrameRegion *vp, short x, short y, Image *img) {
   short imgWidth = imageGetWidth(img);
   short imgHeight = imageGetHeight(img);

   short clipSizeX, clipSizeY, ignoreOffsetX, ignoreOffsetY;
   short borderRight, borderBottom;
   int j, i;
   byte colorBuffer[MAX_IMAGE_WIDTH];
   byte alphaBuffer[MAX_IMAGE_WIDTH];

   ignoreOffsetX = x < 0 ? -x : 0;
   ignoreOffsetY = y < 0 ? -y : 0;

   x += vp->origin_x;
   y += vp->origin_y;

   borderRight = MIN(EGA_RES_WIDTH, vp->origin_x + vp->width);
   borderBottom = MIN(EGA_RES_HEIGHT, vp->origin_y + vp->height);

   clipSizeX = imgWidth;
   clipSizeY = imgHeight;
   if (clipSizeX + x > borderRight) clipSizeX = borderRight - x;
   if (clipSizeY + y > borderBottom) clipSizeY = borderBottom - y;

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

void frameRenderRect(Frame *self, FrameRegion *vp, short left, short top, short right, short bottom, byte color){
   short width = right - left;
   short height = bottom - top;
   short x = left;
   short y = top;
   short clipSizeX, clipSizeY, ignoreOffsetX, ignoreOffsetY;
   int j, i;
   byte colorBuffer[MAX_IMAGE_WIDTH] = { 0 };
   byte alphaBuffer[MAX_IMAGE_WIDTH] = { 0 };
   short borderRight, borderBottom;

   ignoreOffsetX = x < 0 ? -x : 0;
   ignoreOffsetY = y < 0 ? -y : 0;

   x += vp->origin_x;
   y += vp->origin_y;

   borderRight = MIN(EGA_RES_WIDTH, vp->origin_x + vp->width);
   borderBottom = MIN(EGA_RES_HEIGHT, vp->origin_y + vp->height);

   clipSizeX = width;
   clipSizeY = height;
   if (clipSizeX + x > borderRight) clipSizeX = borderRight - x;
   if (clipSizeY + y > borderBottom) clipSizeY = borderBottom - y;

   clipSizeX -= ignoreOffsetX;
   clipSizeY -= ignoreOffsetY;

   if (clipSizeX <= 0 || clipSizeY <= 0) {
      return;
   }

   for (j = 0; j < clipSizeY; ++j) {
      for (i = 0; i < EGA_PLANES; ++i) {
         byte bit = getBitFromArray(&color, i) ? 255 : 0;
         memset(colorBuffer, bit, sizeof(colorBuffer));

         _scanLineRenderImageScanLine(&self->planes[i].lines[j + y + ignoreOffsetY], x + ignoreOffsetX,
            colorBuffer, alphaBuffer, ignoreOffsetX, clipSizeX);
      }
   }
}

void frameRenderPoint(Frame *self, FrameRegion *vp, short x, short y, byte color){
   if (x < 0 || x >= vp->width || y < 0 || y >= vp->height) {
      return;
   }

   x += vp->origin_x;
   y += vp->origin_y;   

   if (x >= 0 && x < EGA_RES_WIDTH && y >= 0 && y < EGA_RES_HEIGHT){
      int j;
      for (j = 0; j < EGA_PLANES; ++j) {
         scanLineSetBit(&self->planes[j].lines[y], x, getBitFromArray(&color, j));
      }
   }
}

void frameRenderLine(Frame *self, FrameRegion *vp, short _x0, short _y0, short _x1, short _y1, byte color){
   short dx = abs(_x1 - _x0);
   short dy = abs(_y1 - _y0);
   short x0, x1, y0, y1;
   float x, y, slope;

   //len=0
   if (!dx && !dy){
      return;
   }

   if (dx > dy){
      if (_x0 > _x1){//flip
         x0 = _x1; y0 = _y1;
         x1 = _x0; y1 = _y0;
      }
      else{
         x0 = _x0; y0 = _y0;
         x1 = _x1; y1 = _y1;
      }

      x = x0;
      y = y0;
      slope = (float)(y1 - y0) / (float)(x1 - x0);
      
      while (x < x1){
         frameRenderPoint(self, vp, (short)x, (short)y, color);

         x += 1.0f;
         y += slope;
      }

      frameRenderPoint(self, vp, (short)x1, (short)y1, color);
   }
   else{
      if (_y0 > _y1){//flip
         x0 = _x1; y0 = _y1;
         x1 = _x0; y1 = _y0;
      }
      else{
         x0 = _x0; y0 = _y0;
         x1 = _x1; y1 = _y1;
      }

      x = x0;
      y = y0;
      slope = (float)(x1 - x0) / (float)(y1 - y);

      while (y < y1){

         frameRenderPoint(self, vp, (short)x, (short)y, color);

         y += 1.0f;
         x += slope;
      }

      frameRenderPoint(self, vp, (short)x1, (short)y1, color);
   }
}

void frameClear(Frame *self, FrameRegion *vp, byte color){
   int i;
   if (vp == FrameRegionFULL) {
      for (i = 0; i < EGA_PLANES; ++i) {
         byte current = getBit(color, i) ? 255 : 0;
         memset((byte*)(self->planes + i), current, EGA_BYTES);
      }
   }
   else {
      frameRenderRect(self, FrameRegionFULL, 
         vp->origin_x, vp->origin_y, 
         vp->origin_x + vp->width, 
         vp->origin_y + vp->height, color);
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
   
   bitBufferDestroy(buffer);
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
   bitBufferDestroy(buff);
}

Palette paletteCreatePartial(byte *data, byte pOffset, byte pCount, byte totalCount){
   Palette r;
   byte i;

   memset(r.colors, EGA_COLOR_UNUSED, EGA_PALETTE_COLORS);

   if(pOffset < 0 || 
      pOffset >= EGA_PALETTE_COLORS || 
      totalCount > EGA_PALETTE_COLORS || 
      pOffset + pCount > EGA_PALETTE_COLORS ||
      pCount < 0) {
         return r;
   }

   for(i = 0; i < totalCount; ++i){
      r.colors[i] = EGA_COLOR_UNDEFINED;
   }

   for(i = 0; i < pCount; ++i){
      r.colors[i] = data[pOffset+i];
   }

   return r;
}

void paletteCopy(Palette *dest, Palette *src){
   memcpy(dest->colors, src->colors, EGA_PALETTE_COLORS);
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

