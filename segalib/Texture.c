#include "EGA.h"
#include "segashared/CheckedMemory.h"
#include "segautils/BitTwiddling.h"
#include "segautils/Defs.h"
#include "segautils/StandardVectors.h"

#include <string.h>
#include <math.h>

struct Texture_t{
   int w, h;
   byte *data;
   size_t size, alloc, byteWidth, planeSize;
   FrameRegion full;
};

static byte *_alphaPlane(Texture *self) { return self->data + self->planeSize * EGA_PLANES; }
static byte *_plane(Texture *self, byte idx) { return self->data + self->planeSize * idx; }
static byte *_scanLine(Texture *self, int y, byte plane) { return _plane(self, plane) + y*self->byteWidth; }
static byte *_alphaScanLine(Texture *self, int y) { return _alphaPlane(self) + y*self->byteWidth; }

Texture *textureCreate(int width, int height) {
   Texture *out = checkedCalloc(1, sizeof(Texture));
   out->full = (FrameRegion) { 0, 0, width, height };
   out->w = width;
   out->h = height;
   out->byteWidth = width % 8 ? ((width >> 1) + 1) : (width >> 1);
   out->planeSize = out->byteWidth * height;
   out->size = out->alloc = out->planeSize * EGA_IMAGE_PLANES;
   out->data = checkedCalloc(1, out->size);

   //need to flip alpha
   memset(_alphaPlane(out), 255, out->planeSize);

   return out;
}

void textureResize(Texture *self, int width, int height) {
   if (width != self->w || height != self->h) {
      size_t bwidth = width % 8 ? ((width >> 1) + 1) : (width >> 1);
      size_t planeSize = bwidth * height;
      size_t newSize = planeSize * EGA_IMAGE_PLANES;

      if (newSize > self->alloc) {
         checkedFree(self->data);
         self->data = checkedCalloc(1, newSize);
         self->alloc = newSize;
      }

      self->full = (FrameRegion) { 0, 0, width, height };
      self->w = width;
      self->h = height;
      self->byteWidth = bwidth;
      self->planeSize = planeSize;
      self->size = newSize;
   }

   memset(self->data, 0, self->planeSize * EGA_PLANES);
   memset(_alphaPlane(self), 255, self->planeSize);
}



void textureDestroy(Texture *self) {
   checkedFree(self->data);
   checkedFree(self);
}

int textureGetWidth(Texture *self) { return self->w; }
int textureGetHeight(Texture *self) { return self->h; }

byte *textureGetScanline(Texture *self, byte plane, int y) { return _scanLine(self, y, plane); }
byte *textureGetAlphaScanline(Texture *self, int y) { return _alphaScanLine(self, y); }

FrameRegion *textureGetFullRegion(Texture *self) { return &self->full; }

Texture *imageCreateTexture(Image *self) {
   Texture *out = textureCreate(imageGetWidth(self), imageGetHeight(self));
   int plane, scanline;
   for (scanline = 0; scanline < out->h; ++scanline) {
      //copy out the color
      for (plane = 0; plane < EGA_PLANES; ++plane) {
         //plane+1 because images use their first plane for alpha
         imageScanLineRender(imageGetScanLine(self, scanline, plane + 1), _scanLine(out, scanline, plane));
      }

      //copy out the alpha
      imageScanLineRender(imageGetScanLine(self, scanline, 0), _alphaScanLine(out, scanline));
   }

   return out;
}

static byte _scanLineGetBit(byte *sl, short position) {
   return  getBitFromArray(sl, position);
}

static void _scanLineSetBit(byte *sl, short position, byte value) {
   setBitInArray(sl, position, value);
}

static void _renderAlphaBit(byte *dest, byte *color, int *texX, int *x) {
   byte c = _scanLineGetBit(color, *texX);//c
   byte s = _scanLineGetBit(dest, *x);//screen
   _scanLineSetBit(dest, *x, (s & c));
   ++(*x);
   ++(*texX);
}

static void _renderAlpha8Bits(byte *dest, byte *color, int *texX, int *x, int byteRun) {
   int i;
   int frac = (*texX) % 8;  //how far we are into the image 

                            //1 << 3 == 8
   uint8_t *screenArr = ((uint8_t*)dest) + ((*x) >> 3);
   uint8_t *imgArr = ((uint8_t*)color) + ((*texX) >> 3);
   
   if (!frac) { //fast path!
      for (i = 0; i < byteRun; ++i, (*x) += 8, (*texX) += 8) {
         *screenArr++ &= *imgArr++;
      }
   }
   else {//slow path...
      for (i = 0; i < byteRun; ++i, (*x) += 8, (*texX) += 8) {
         *screenArr &= (*imgArr >> frac) | ((*(imgArr + 1)) << (8 - frac));
         ++imgArr;
         ++screenArr;
      }
   }
}

static void _renderAlphaPartialByte(byte *dest, byte *color, int texX) {
   int i;
   int frac = texX % 8;
   uint8_t *screenArr = (uint8_t*)dest;
   uint8_t *imgArr = ((uint8_t*)color) + (texX >> 3);

   if (!frac) {
      *screenArr &= *imgArr;
   }
   else {
      *screenArr &= (*imgArr >> frac) | ((*(imgArr + 1)) << (8 - frac));
   }
}

static void _renderAlpha32Bits(byte *dest, byte *color, int *texX, int *x, int intRun) {
   int i;
   int intBits = sizeof(uint32_t) * 8;
   int frac = *texX % intBits;  //how far we are into the image 

                                //1 << 5 == 32
   uint32_t *screenArr = ((uint32_t*)dest) + (*x >> 5);
   uint32_t *imgArr = ((uint32_t*)color) + (*texX >> 5);

   if (!frac) { //fast path!
      for (i = 0; i < intRun; ++i, *x += intBits, *texX += intBits) {
         *screenArr++ &= *imgArr++;
      }
   }
   else {//slow path...
      for (i = 0; i < intRun; ++i, *x += intBits, *texX += intBits) {
         *screenArr &= (*imgArr >> frac) | ((*(imgArr + 1)) << (intBits - frac));
         ++imgArr;
         ++screenArr;
      }
   }



}

static void _renderAlphaScanLine(byte *dest, int x, byte *color, int bitOffset, int width) {
   int intRun, byteRun, alignRun;
   int texX = bitOffset;
   int last = x + width;
   static const byte mask = 255;

   int intBits = sizeof(uint32_t) * 8;
   int alignedBits = x % 8;

   if (alignedBits) {//not aligned
      int xover8 = x >> 3;
      byte current = *(dest + xover8);
      byte buffer = current;
      byte untilAligned = MIN(8 - alignedBits, last - x);

      //if the total draw doesnt even get us to aligned, just draw
      //using renderbit
      if (alignedBits + untilAligned == 8) {
         int postoffset = 0;
         bool texunderflow = texX < alignedBits;
         int tempTexX = MAX(0, texX - alignedBits);

         if (texunderflow) {
            postoffset = alignedBits - texX;
            buffer >>= postoffset;
         }

         _renderAlphaPartialByte(&buffer, color, tempTexX);

         if (texunderflow) {
            buffer <<= postoffset;
         }

         *(dest + xover8) = (buffer & (mask << alignedBits)) | (current & (mask >> untilAligned));

         x += untilAligned;
         texX += untilAligned;
      }
      else {
         while (x < last && alignedBits++ < 8) {
            _renderAlphaBit(dest, color, &texX, &x);
         }
      }
   }

   if (x == last) {
      return;
   }

   byteRun = (last - x) / 8;
   alignRun = (4 - ((x % intBits) / 8)) % 4;
   _renderAlpha8Bits(dest, color, &texX, &x, MIN(alignRun, byteRun));

   intRun = (last - x) / intBits;
   _renderAlpha32Bits(dest, color, &texX, &x, intRun);

   byteRun = (last - x) / 8;
   _renderAlpha8Bits(dest, color, &texX, &x, byteRun);

   if (x < last) {
      int xover8 = x >> 3;
      int remaining = last - x;
      byte current = *(dest + xover8);
      byte buffer = current;
      int tempTexX = texX;

      _renderAlphaPartialByte(&buffer, color, tempTexX);
      *(dest + xover8) = (buffer & (mask >> (8 - remaining))) | (current & (mask << remaining));
   }

}


static void _renderBit(byte *dest, byte *color, byte *alpha, int *texX, int *x) {
   //setbits...
   byte t = _scanLineGetBit(alpha, *texX);//trans
   byte c = _scanLineGetBit(color, *texX);//c
   byte s = _scanLineGetBit(dest, *x);//screen

                                       //if alpha, use color, else use screen
   _scanLineSetBit(dest, *x, ((s & t) | c));

   ++(*x);
   ++(*texX);
}

static void _render8Bits(byte *dest, byte *color, byte *alpha, int *texX, int *x, int byteRun) {
   int i;
   int frac = (*texX) % 8;  //how far we are into the image 

                            //1 << 3 == 8
   uint8_t *screenArr = ((uint8_t*)dest) + ((*x) >> 3);
   uint8_t *imgArr = ((uint8_t*)color) + ((*texX) >> 3);
   uint8_t *alphaArr = ((uint8_t*)alpha) + ((*texX) >> 3);

   if (!frac) { //fast path!
      for (i = 0; i < byteRun; ++i, (*x) += 8, (*texX) += 8) {
         //simply aligned, do regular int operations
         *screenArr &= *alphaArr++;
         *screenArr++ |= *imgArr++;
      }
   }
   else {//slow path...
      for (i = 0; i < byteRun; ++i, (*x) += 8, (*texX) += 8) {
         //since this is unaligned, we do it in two slices.  Let's build the image bits into an aligned int32_t.
         *screenArr &= (*alphaArr >> frac) | ((*(alphaArr + 1)) << (8 - frac));
         *screenArr |= (*imgArr >> frac) | ((*(imgArr + 1)) << (8 - frac));

         ++imgArr;
         ++screenArr;
         ++alphaArr;
      }
   }

}

static void _renderPartialByte(byte *dest, byte *color, byte *alpha, int texX) {
   int i;
   int frac = texX % 8;  //how far we are into the image 

                            //1 << 3 == 8
   uint8_t *screenArr = (uint8_t*)dest;
   uint8_t *imgArr = ((uint8_t*)color) + (texX >> 3);
   uint8_t *alphaArr = ((uint8_t*)alpha) + (texX >> 3);

   if (!frac) { //fast path!
      //simply aligned, do regular int operations
      *screenArr &= *alphaArr;
      *screenArr |= *imgArr;
   }
   else {//slow path...
      //since this is unaligned, we do it in two slices.  Let's build the image bits into an aligned int32_t.
      *screenArr &= (*alphaArr >> frac) | ((*(alphaArr + 1)) << (8 - frac));
      *screenArr |= (*imgArr >> frac) | ((*(imgArr + 1)) << (8 - frac));
   }
}

static void _render32Bits(byte *dest, byte *color, byte *alpha, int *texX, int *x, int intRun) {
   int i;
   int intBits = sizeof(uint32_t) * 8;
   int frac = *texX % intBits;  //how far we are into the image 

                                //1 << 5 == 32
   uint32_t *screenArr = ((uint32_t*)dest) + (*x >> 5);
   uint32_t *imgArr = ((uint32_t*)color) + (*texX >> 5);
   uint32_t *alphaArr = ((uint32_t*)alpha) + (*texX >> 5);

   if (!frac) { //fast path!
      for (i = 0; i < intRun; ++i, *x += intBits, *texX += intBits) {
         //simply aligned, do regular int operations
         *screenArr &= *alphaArr++;
         *screenArr++ |= *imgArr++;
      }
   }
   else {//slow path...
      for (i = 0; i < intRun; ++i, *x += intBits, *texX += intBits) {
         //since this is unaligned, we do it in two slices.  Let's build the image bits into an aligned int32_t.
         *screenArr &= (*alphaArr >> frac) | ((*(alphaArr + 1)) << (intBits - frac));
         *screenArr |= (*imgArr >> frac) | ((*(imgArr + 1)) << (intBits - frac));

         ++imgArr;
         ++screenArr;
         ++alphaArr;
      }
   }
   

   
}

static void _renderScanLine(byte *dest, int x, byte *color, byte *alpha, int bitOffset, int width) {
   int intRun, byteRun, alignRun;
   int texX = bitOffset;
   int last = x + width;
   static const byte mask = 255;

   int intBits = sizeof(uint32_t) * 8;
   byte alignedBits = x % 8;

   if (alignedBits) {//not aligned
      int xover8 = x >> 3;
      byte current = *(dest + xover8);
      byte buffer = current;
      byte untilAligned = MIN(8 - alignedBits, last - x);

      //if the total draw doesnt even get us to aligned, just draw
      //using renderbit
      if (alignedBits + untilAligned == 8) {         
         int postoffset = 0;
         bool texunderflow = texX < alignedBits;
         int tempTexX = MAX(0, texX - alignedBits);

         if (texunderflow) {
            postoffset = alignedBits - texX;
            buffer >>= postoffset;
         }

         _renderPartialByte(&buffer, color, alpha, tempTexX);

         if (texunderflow) {
            buffer <<= postoffset;
         }

         *(dest + xover8) = (buffer & (mask << alignedBits)) | (current & (mask >> untilAligned));

         x += untilAligned;
         texX += untilAligned;
      }
      else {
         while (x < last && alignedBits++ < 8) {
            _renderBit(dest, color, alpha, &texX, &x);
         }
      }
   }

   if (x == last) {
      return;
   }

   byteRun = (last - x) / 8;
   alignRun = (4 - ((x % intBits) / 8)) % 4;
   _render8Bits(dest, color, alpha, &texX, &x, MIN(alignRun, byteRun));

   intRun = (last - x) / intBits;
   _render32Bits(dest, color, alpha, &texX, &x, intRun);

   byteRun = (last - x) / 8;
   _render8Bits(dest, color, alpha, &texX, &x, byteRun);

   if (x < last) {
      int xover8 = x >> 3;
      int remaining = last - x; 
      byte current = *(dest + xover8);
      byte buffer = current;
      int tempTexX = texX;

      _renderPartialByte(&buffer, color, alpha, tempTexX);
      *(dest + xover8) = (buffer & (mask >> (8 - remaining))) | (current & (mask << remaining));
   }
}

void textureClear(Texture *self, FrameRegion *vp, byte color) {
   if (!vp) {
      int i;

      memset(_alphaPlane(self), 0, self->planeSize);
     
      for (i = 0; i < EGA_PLANES; ++i) {
         byte current = getBit(color, i) ? 255 : 0;
         memset(_plane(self, i), current, self->planeSize);
      }
   }
   else {
      textureRenderRect(self, NULL,
         vp->origin_x, vp->origin_y,
         vp->origin_x + vp->width,
         vp->origin_y + vp->height, color);
   }
}
void textureClearAlpha(Texture *self) {

   memset(self->data, 0, self->planeSize*EGA_IMAGE_PLANES);
   memset(_alphaPlane(self), 255, self->planeSize);
}

void textureRenderTexture(Texture *self, FrameRegion *vp, int x, int y, Texture *tex) {
   int texWidth = tex->w;
   int texHeight = tex->h;

   int clipSizeX, clipSizeY, ignoreOffsetX, ignoreOffsetY;
   int borderRight, borderBottom;
   int yIter, plane;

   byte *color = 0, *alpha = 0;

   if (!vp) {
      vp = &self->full;
   }

   ignoreOffsetX = x < 0 ? -x : 0;
   ignoreOffsetY = y < 0 ? -y : 0;

   x += vp->origin_x;
   y += vp->origin_y;

   borderRight = MIN(self->w, vp->origin_x + vp->width);
   borderBottom = MIN(self->h, vp->origin_y + vp->height);

   clipSizeX = texWidth;
   clipSizeY = texHeight;
   if (clipSizeX + x > borderRight) clipSizeX = borderRight - x;
   if (clipSizeY + y > borderBottom) clipSizeY = borderBottom - y;

   clipSizeX -= ignoreOffsetX;
   clipSizeY -= ignoreOffsetY;

   if (clipSizeX <= 0 || clipSizeY <= 0) {
      return;
   }

   for (yIter = 0; yIter < clipSizeY; ++yIter) {
      int yLine = yIter + ignoreOffsetY;

      alpha = _alphaScanLine(tex, yLine);

      for (plane = 0; plane < EGA_PLANES; ++plane) {
         color = _scanLine(tex, yLine, plane);
         _renderScanLine(_scanLine(self, y + yLine, plane), x + ignoreOffsetX, color, alpha, ignoreOffsetX, clipSizeX);
      }

      //now we 'render' the alpha scanline onto our own alpha plane!
      _renderAlphaScanLine(_alphaScanLine(self, y + yLine), x + ignoreOffsetX, alpha, ignoreOffsetX, clipSizeX);
   }
}

void textureRenderTexturePartial(Texture *self, FrameRegion *vp, int x, int y, Texture *tex, int texX, int texY, int subtexWidth, int subtexHeight) {
   int texWidth = MIN(subtexWidth, tex->w);
   int texHeight = MIN(subtexHeight, tex->h);

   int clipSizeX, clipSizeY, ignoreOffsetX, ignoreOffsetY;
   int borderRight, borderBottom;
   int yIter, plane;

   byte *color = 0, *alpha = 0;

   if (!vp) {
      vp = &self->full;
   }

   ignoreOffsetX = x < 0 ? -x : 0;
   ignoreOffsetY = y < 0 ? -y : 0;

   x += vp->origin_x;
   y += vp->origin_y;

   borderRight = MIN(self->w, vp->origin_x + vp->width);
   borderBottom = MIN(self->h, vp->origin_y + vp->height);

   clipSizeX = texWidth;
   clipSizeY = texHeight;
   if (clipSizeX + x > borderRight) clipSizeX = borderRight - x;
   if (clipSizeY + y > borderBottom) clipSizeY = borderBottom - y;

   clipSizeX -= ignoreOffsetX;
   clipSizeY -= ignoreOffsetY;

   if (clipSizeX <= 0 || clipSizeY <= 0) {
      return;
   }

   for (yIter = 0; yIter < clipSizeY && yIter + ignoreOffsetY < texHeight; ++yIter) {
      int yLineSrc = yIter + ignoreOffsetY + texY;
      int yLineDest = yIter + y + ignoreOffsetY;

      alpha = _alphaScanLine(tex, yLineSrc);

      for (plane = 0; plane < EGA_PLANES; ++plane) {
         color = _scanLine(tex, yLineSrc, plane);
         _renderScanLine(_scanLine(self, yLineDest, plane), x + ignoreOffsetX, color, alpha, texX + ignoreOffsetX, clipSizeX);
      }

      //now we 'render' the alpha scanline onto our own alpha plane!
      _renderAlphaScanLine(_alphaScanLine(self, yLineDest), x + ignoreOffsetX, alpha, ignoreOffsetX, clipSizeX);
   }
}

void textureRenderPoint(Texture *self, FrameRegion *vp, int x, int y, byte color) {
   int plane;
   if (!vp) { vp = &self->full; }

   if (x < 0 || x >= vp->width || y < 0 || y >= vp->height) {
      return;
   }

   x += vp->origin_x;
   y += vp->origin_y;

   if (x >= 0 && x < self->w && y >= 0 && y < self->h) {
      for (plane = 0; plane < EGA_PLANES; ++plane) {
         _scanLineSetBit(_scanLine(self, y, plane), x, getBitFromArray(&color, plane));
      }
      _scanLineSetBit(_alphaScanLine(self, y), x, 0);
   }
}

void textureRenderLine(Texture *self, FrameRegion *vp, int _x0, int _y0, int _x1, int _y1, byte color) {
   int dx = abs(_x1 - _x0);
   int dy = abs(_y1 - _y0);
   int x0, x1, y0, y1;
   float x, y, slope;

   //len=0
   if (!dx && !dy) {
      return;
   }

   if (dx > dy) {
      if (_x0 > _x1) {//flip
         x0 = _x1; y0 = _y1;
         x1 = _x0; y1 = _y0;
      }
      else {
         x0 = _x0; y0 = _y0;
         x1 = _x1; y1 = _y1;
      }

      x = x0;
      y = y0;
      slope = (float)(y1 - y0) / (float)(x1 - x0);

      while (x < x1) {
         textureRenderPoint(self, vp, x, y, color);

         x += 1.0f;
         y += slope;
      }

      textureRenderPoint(self, vp, x1, y1, color);
   }
   else {
      if (_y0 > _y1) {//flip
         x0 = _x1; y0 = _y1;
         x1 = _x0; y1 = _y0;
      }
      else {
         x0 = _x0; y0 = _y0;
         x1 = _x1; y1 = _y1;
      }

      x = x0;
      y = y0;
      slope = (float)(x1 - x0) / (float)(y1 - y);

      while (y < y1) {

         textureRenderPoint(self, vp, x, y, color);

         y += 1.0f;
         x += slope;
      }

      textureRenderPoint(self, vp, x1, y1, color);
   }
}

void textureRenderLineRect(Texture *self, FrameRegion *vp, int left, int top, int right, int bottom, byte color) {
   textureRenderLine(self, vp, left, top, right, top, color);
   textureRenderLine(self, vp, left, bottom, right, bottom, color);
   textureRenderLine(self, vp, left, top, left, bottom, color);
   textureRenderLine(self, vp, right, top, right, bottom, color);
}
void textureRenderRect(Texture *self, FrameRegion *vp, int left, int top, int right, int bottom, byte color) {
   int width = right - left;
   int height = bottom - top;
   int x = left;
   int y = top;

   int clipSizeX, clipSizeY, ignoreOffsetX, ignoreOffsetY;
   int borderRight, borderBottom;
   int yIter, plane;

   byte colorBuffer[MAX_IMAGE_WIDTH] = { 0 };
   byte alphaBuffer[MAX_IMAGE_WIDTH] = { 0 };


   if (!vp) {
      vp = &self->full;
   }

   ignoreOffsetX = x < 0 ? -x : 0;
   ignoreOffsetY = y < 0 ? -y : 0;

   x += vp->origin_x;
   y += vp->origin_y;

   borderRight = MIN(self->w, vp->origin_x + vp->width);
   borderBottom = MIN(self->h, vp->origin_y + vp->height);

   clipSizeX = width;
   clipSizeY = height;
   if (clipSizeX + x > borderRight) clipSizeX = borderRight - x;
   if (clipSizeY + y > borderBottom) clipSizeY = borderBottom - y;

   clipSizeX -= ignoreOffsetX;
   clipSizeY -= ignoreOffsetY;

   if (clipSizeX <= 0 || clipSizeY <= 0) {
      return;
   }

   for (yIter = 0; yIter < clipSizeY; ++yIter) {
      int yLine = yIter + ignoreOffsetY;

      memset(alphaBuffer, 0, sizeof(alphaBuffer));

      for (plane = 0; plane < EGA_PLANES; ++plane) {
         byte bit = getBitFromArray(&color, plane) ? 255 : 0;
         memset(colorBuffer, bit, sizeof(colorBuffer));

         _renderScanLine(_scanLine(self, y + yLine, plane), x + ignoreOffsetX, colorBuffer, alphaBuffer, ignoreOffsetX, clipSizeX);
      }

      //now we 'render' the alpha scanline onto our own alpha plane!
      _renderAlphaScanLine(_alphaScanLine(self, y + yLine), x + ignoreOffsetX, alphaBuffer, ignoreOffsetX, clipSizeX);
   }
}

void textureRenderCircle(Texture *self, FrameRegion *vp, int xc, int yc, int radius, byte color){
   int x = 0;
   int y = radius;
   int delta = 2 - 2 * radius;
   int error = 0;

   while (y >= 0)
   {
      textureRenderPoint(self, vp, xc + x, yc + y, color);
      textureRenderPoint(self, vp, xc - x, yc + y, color);
      textureRenderPoint(self, vp, xc + x, yc - y, color);
      textureRenderPoint(self, vp, xc - x, yc - y, color);

      error = 2 * (delta + y) - 1;
      if (delta < 0 && error <= 0) {
         ++x;
         delta += 2 * x + 1;
         continue;
      }
      error = 2 * (delta - x) - 1;
      if (delta > 0 && error > 0) {
         --y;
         delta += 1 - 2 * y;
         continue;
      }
      ++x;
      delta += 2 * (x - y);
      --y;
   }
}

void textureRenderEllipse(Texture *self, FrameRegion *vp, int xc, int yc, int width, int height, byte color){
   int a2 = width * width;
   int b2 = height * height;
   int fa2 = 4 * a2, fb2 = 4 * b2;
   int x, y, sigma;

   if (width == 0 || height == 0) {
      return;
   }

   /* first half */
   for (x = 0, y = height, sigma = 2 * b2 + a2*(1 - 2 * height); b2*x <= a2*y; x++)
   {
      textureRenderPoint(self, vp, xc + x, yc + y, color);
      textureRenderPoint(self, vp, xc - x, yc + y, color);
      textureRenderPoint(self, vp, xc + x, yc - y, color);
      textureRenderPoint(self, vp, xc - x, yc - y, color);
      if (sigma >= 0)
      {
         sigma += fa2 * (1 - y);
         y--;
      }
      sigma += b2 * ((4 * x) + 6);
   }

   /* second half */
   for (x = width, y = 0, sigma = 2 * a2 + b2*(1 - 2 * width); a2*y <= b2*x; y++)
   {
      textureRenderPoint(self, vp, xc + x, yc + y, color);
      textureRenderPoint(self, vp, xc - x, yc + y, color);
      textureRenderPoint(self, vp, xc + x, yc - y, color);
      textureRenderPoint(self, vp, xc - x, yc - y, color);
      if (sigma >= 0)
      {
         sigma += fb2 * (1 - x);
         x--;
      }
      sigma += a2 * ((4 * y) + 6);
   }
}

//void drawEllipse(Texture *self, FrameRegion *vp, int xc, int yc, int x, int y, byte color)
//{
//   /*This function plots a pixel at coordinates(x,y) specified by first 2 arguments and third argument specifies the color of the pixel*/
//   textureRenderPoint(self, vp, xc + x, yc + y, color);
//   textureRenderPoint(self, vp, xc - x, yc + y, color);
//   textureRenderPoint(self, vp, xc + x, yc - y, color);
//   textureRenderPoint(self, vp, xc - x, yc - y, color);
//}
//
///* Function to draw ellipse using the mid point algorithm */
//
//void textureRenderEllipse(Texture *self, FrameRegion *vp, int xc, int yc, int rx, int ry, byte color)
//{
//   float rxSq = rx * rx;
//   float rySq = ry * ry;
//   float x = 0, y = ry, p;
//   float px = 0, py = 2 * rxSq * y;
//
//   drawEllipse(self, vp, xc, yc, x, y, color);
//
//   //Region 1
//   p = rySq - (rxSq * ry) + (0.25 * rxSq);
//   while (px < py)
//   {
//      x++;
//      px = px + 2 * rySq;
//      if (p < 0)
//         p = p + rySq + px;
//      else
//      {
//         y--;
//         py = py - 2 * rxSq;
//         p = p + rySq + px - py;
//      }
//      drawEllipse(self, vp, xc, yc, x, y, color);
//   }
//
//   //Region 2
//   p = rySq*(x + 0.5)*(x + 0.5) + rxSq*(y - 1)*(y - 1) - rxSq*rySq;
//   while (y > 0)
//   {
//      y--;
//      py = py - 2 * rxSq;
//      if (p > 0)
//         p = p + rxSq - py;
//      else
//      {
//         x++;
//         px = px + 2 * rySq;
//         p = p + rxSq - py + px;
//      }
//      drawEllipse(self, vp, xc, yc, x, y, color);
//   }
//}



void textureRenderEllipseQB(Texture *self, FrameRegion *vp, int xc, int yc, int radius, byte color, double aspect) {
   if (aspect < 0) {
      aspect = 4.0 * (EGA_RES_HEIGHT / (double)EGA_RES_WIDTH  ) / 3.0;
   }
   
   textureRenderEllipse(self, vp, xc, yc, radius, round(radius * aspect), color);
}

byte textureGetColorAt(Texture *self, FrameRegion *vp, int x, int y) {
   byte out = 0;
   int plane = 0;

   if (!vp) { vp = &self->full; }

   if (x < 0 || x >= vp->width || y < 0 || y >= vp->height) {
      return 0;
   }

   x += vp->origin_x;
   y += vp->origin_y;

   if (x >= 0 && x < self->w && y >= 0 && y < self->h) {
      if (getBitFromArray(_alphaScanLine(self, y), x)) {
         //1 means invis, return black
         return 0;
      }

      for (plane = 0; plane < EGA_PLANES; ++plane) {
         setBitInArray(&out, plane, getBitFromArray(_scanLine(self, y, plane), x));
      }
   }
   
   return out;
}


static byte _colorAt(Texture *self, int x, int y) {
   byte out = 0;
   int plane = 0;
   if (getBitFromArray(_alphaScanLine(self, y), x)) {
      //1 means invis, return black
      return 0;
   }

   for (plane = 0; plane < EGA_PLANES; ++plane) {
      setBitInArray(&out, plane, getBitFromArray(_scanLine(self, y, plane), x));
   }
   return out;
}

static void _addTileToFloodFill(Texture *self, FrameRegion *vp, byte border, int x, int y, vec(size_t) *openList, vec(size_t) *closedList) {
   int vpx = x + vp->origin_x;
   int vpy = y + vp->origin_y;
   size_t pxid = vpy * self->w + vpx;

   if (!(x < 0 || x >= vp->width || y < 0 || y >= vp->height)) {
      if (vpx >= 0 && vpx < self->w && vpy >= 0 && vpy < self->h) {
         if (_colorAt(self, vpx, vpy) != border) {            
            vecPushBack(size_t)(openList, &pxid);
            return;
         }
      }
   }

   vecPushBack(size_t)(closedList, &pxid);
}

static void _floodFill(Texture *self, FrameRegion *vp, byte color, byte border, vec(size_t) *openList, vec(size_t) *closedList) {


   size_t pxid = *vecAt(size_t)(openList, 0);
   int x = pxid % self->w;
   int y = pxid / self->w;

   int vpx = x + vp->origin_x;
   int vpy = y + vp->origin_y;

   if (!(x < 0 || x >= vp->width || y < 0 || y >= vp->height)) {
      if (vpx >= 0 && vpx < self->w && vpy >= 0 && vpy < self->h) {
         if (_colorAt(self, vpx, vpy) != border) {
            textureRenderPoint(self, vp, x, y, color);

            _addTileToFloodFill(self, vp, border, x - 1, y, openList, closedList);
            _addTileToFloodFill(self, vp, border, x, y - 1, openList, closedList);
            _addTileToFloodFill(self, vp, border, x + 1, y, openList, closedList);
            _addTileToFloodFill(self, vp, border, x, y + 1, openList, closedList);
         }
      }
   }

   vecRemoveAt(size_t)(openList, 0);
   vecPushBack(size_t)(closedList, &pxid);
}


void textureRenderFloodFill(Texture *self, FrameRegion *vp, int x, int y, byte color, byte borderColor) {
   if (!vp) { vp = &self->full; }

   if (x < 0 || x >= vp->width || y < 0 || y >= vp->height) {
      return;
   }

   int vpx = x + vp->origin_x;
   int vpy = y + vp->origin_y;
   size_t pxid = vpy * self->w + vpx;

   vec(size_t) *openList = vecCreate(size_t)(NULL);
   vec(size_t) *closedList = vecCreate(size_t)(NULL);

   vecPushBack(size_t)(openList, &pxid);
   while (!vecIsEmpty(size_t)(openList)) {
      _floodFill(self, vp, color, borderColor, openList, closedList);
   }

   vecDestroy(size_t)(openList);
   vecDestroy(size_t)(closedList);




   //if (vpx >= 0 && vpx < self->w && vpy >= 0 && vpy < self->h) {
   //   byte oColor = _colorAt(self, vpx, vpy);
   //   if (oColor == borderColor) {
   //      return;
   //   }

   //   textureRenderPoint(self, vp, x, y, color);
   //   textureRenderFloodFill(self, vp, x + 1, y, color, borderColor);
   //   textureRenderFloodFill(self, vp, x - 1, y, color, borderColor);
   //   textureRenderFloodFill(self, vp, x, y+1, color, borderColor);
   //   textureRenderFloodFill(self, vp, x, y-1, color, borderColor);
   //}
}

void frameRenderTexture(Frame *self, FrameRegion *vp, short x, short y, Texture *tex) {
   int texWidth = tex->w;
   int texHeight = tex->h;

   int clipSizeX, clipSizeY, ignoreOffsetX, ignoreOffsetY;
   int borderRight, borderBottom;
   int yIter, plane;

   byte *color = 0, *alpha = 0;

   ignoreOffsetX = x < 0 ? -x : 0;
   ignoreOffsetY = y < 0 ? -y : 0;

   x += vp->origin_x;
   y += vp->origin_y;

   borderRight = MIN(EGA_RES_WIDTH, vp->origin_x + vp->width);
   borderBottom = MIN(EGA_RES_HEIGHT, vp->origin_y + vp->height);

   clipSizeX = texWidth;
   clipSizeY = texHeight;
   if (clipSizeX + x > borderRight) clipSizeX = borderRight - x;
   if (clipSizeY + y > borderBottom) clipSizeY = borderBottom - y;

   clipSizeX -= ignoreOffsetX;
   clipSizeY -= ignoreOffsetY;

   if (clipSizeX <= 0 || clipSizeY <= 0) {
      return;
   }

   for (yIter = 0; yIter < clipSizeY; ++yIter) {
      int yLine = yIter + ignoreOffsetY;
      alpha = _alphaScanLine(tex, yLine);

      for (plane = 0; plane < EGA_PLANES; ++plane) {
         color = _scanLine(tex, yLine, plane);

         _renderScanLine(&self->planes[plane].lines[y + yLine], x + ignoreOffsetX, color, alpha, ignoreOffsetX, clipSizeX);
      }
   }
}