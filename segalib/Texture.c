#include "EGA.h"
#include "segashared/CheckedMemory.h"
#include "segautils/BitTwiddling.h"
#include "segautils/Defs.h"

#include <string.h>

struct Texture_t{
   int w, h;
   byte *data;
   size_t size, byteWidth, planeSize;
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
   out->size = out->planeSize * EGA_IMAGE_PLANES;
   out->data = checkedCalloc(1, out->size);

   //need to flip alpha
   memset(_alphaPlane(out), 255, out->planeSize);

   return out;
}



void textureDestroy(Texture *self) {
   checkedFree(self->data);
   checkedFree(self);
}

int textureGetWidth(Texture *self) { return self->w; }
int textureGetHeight(Texture *self) { return self->h; }



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

   int intBits = sizeof(uint32_t) * 8;
   int alignedBits = x % 8;

   if (alignedBits) {
      while (x < last && alignedBits++ < 8) {
         _renderAlphaBit(dest, color, &texX, &x);
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

   while (x < last) {
      _renderAlphaBit(dest, color, &texX, &x);
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

   int intBits = sizeof(uint32_t) * 8;
   int alignedBits = x % 8;

   if (alignedBits) {
      while (x < last && alignedBits++ < 8) {
         _renderBit(dest, color, alpha, &texX, &x);
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

   while (x < last) {
      _renderBit(dest, color, alpha, &texX, &x);
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

void textureRenderTexture(Texture *self, FrameRegion *vp, short x, short y, Texture *tex) {
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

void textureRenderTexturePartial(Texture *self, FrameRegion *vp, short x, short y, Texture *tex, short imgX, short imgY, short imgWidth, short imgHeight) {

}

void textureRenderPoint(Texture *self, FrameRegion *vp, short x, short y, byte color) {}
void textureRenderLine(Texture *self, FrameRegion *vp, short x1, short y1, short x2, short y2, byte color) {}
void textureRenderLineRect(Texture *self, FrameRegion *vp, short left, short top, short right, short bottom, byte color) {}
void textureRenderRect(Texture *self, FrameRegion *vp, short left, short top, short right, short bottom, byte color) {}
void textureRenderText(Texture *texture, const char *text, short x, short y, Font *font) {}
void textureRenderTextWithoutSpaces(Texture *texture, const char *text, short x, short y, Font *font) {}

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