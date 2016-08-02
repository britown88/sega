#include "EGA.h"
#include "segautils\BitBuffer.h"
#include <malloc.h>
#include "segashared\CheckedMemory.h"
#include "segautils\Defs.h"
#include "segautils\BitTwiddling.h"

#include <stdint.h>
#include <stdio.h>

void imageScanLineDestroy(ImageScanLine *self) {
   if(self) {
      self->vTable->destroy(self);
   }
}
void imageScanLineRender(ImageScanLine *self, byte *output){
   if(self) {
      self->vTable->render(self, output);
   }
}
void imageScanLineSerialize(ImageScanLine *self, BitBuffer *dest) {
   if(self) {
      self->vTable->serialize(self, dest);
   }
}
short imageScanLineGetBitCount(ImageScanLine *self) {
   if(self) {
      return self->vTable->getBitCount(self);
   }
   return 0;
}

typedef ImageScanLine** ImageScanLineArray;

struct Image_t {
   short width, height;
   ImageScanLineArray planes[EGA_PLANES + 1];
};

void imageRenderToFlat(Image *self, FlatImage *dest){
   byte i = 0;
   short y = 0;

   dest->width = self->width;
   dest->height = self->height;

   for (i = 0; i < EGA_IMAGE_PLANES; ++i){
      for (y = 0; y < self->height; ++y){
         imageScanLineRender(imageGetScanLine(self, y, i), dest->planes[i].lines[y].pixels);
      }
   }
}

Image *imageCreate(short width, short height) {
   Image *r = checkedCalloc(1, sizeof(Image));
   int i;

   r->width = width;
   r->height = height;

   for(i = 0; i < EGA_PLANES + 1; ++i) {
      r->planes[i] = checkedCalloc(1, sizeof(ImageScanLine*) * height);
   }

   return r;
}

Image *_imageDeserializeEX(byte *f, long fSize, int optimize){
   BitBuffer *buffer; 
   short width, height;
   int y, i;
   Image *img;

   if (!f) {
      return NULL;
   }

   buffer = bitBufferCreate(f, 1);

   width = bitBufferReadShort(buffer);
   height = bitBufferReadShort(buffer);

   img = imageCreate(width, height);

   for (i = 0; i < EGA_IMAGE_PLANES; ++i) {
      for (y = 0; y < height; ++y){
         ImageScanLine *scanLine = NULL;
         byte typeID = 0;
         bitBufferReadBits(buffer, &typeID, 2);

         switch (typeID) {
         case scanline_SOLID:
            scanLine = createSolidScanLineFromBB(buffer, width);
            break;
         case scanline_RLE:
            scanLine = createRLEScanLineFromBB(buffer);
            break;
         case scanline_UNCOMPRESSED:
            scanLine = createUncompressedScanLineFromBB(buffer);
            break;
         }

         imageSetScanLine(img, y, i, scanLine);
      }
   }

   //alpha computation postpass
   if (optimize){
      int plane, y, x;
      int intCount = minIntCount(width);
      int32_t alphaBuffer[MAX_IMAGE_WIDTH / sizeof(int32_t)] = { 0 };
      int32_t readBuffer[MAX_IMAGE_WIDTH / sizeof(int32_t)] = { 0 };

      for (y = 0; y < height; ++y){
         ImageScanLine *alpha = imageGetScanLine(img, y, 0);
         imageScanLineRender(alpha, (byte*)alphaBuffer);

         for (plane = 0; plane < EGA_PLANES; ++plane){
            ImageScanLine *line = imageGetScanLine(img, y, plane + 1);
            imageScanLineRender(line, (byte*)readBuffer);

            //pre-bake-in alpha
            for (x = 0; x < intCount; ++x){
               readBuffer[x] &= alphaBuffer[x];
            }

            //replace old scnaline with an uncompressed one from the new buffer
            line = createUncompressedScanLine(width, (byte*)readBuffer);
            imageSetScanLine(img, y, plane+1, line);
         }

         //inverse alpha
         for (x = 0; x < intCount; ++x){
            alphaBuffer[x] = ~alphaBuffer[x];
         }

         //replace  alpha line
         alpha = createUncompressedScanLine(width, (byte*)alphaBuffer);
         imageSetScanLine(img, y, 0, alpha);
      }
   }

   bitBufferDestroy(buffer);
   return img;
}

Image *_imageDeserializeEXFile(const char*path, int optimize) {
   long size;
   byte *f = readFullFile(path, &size);
   return _imageDeserializeEX(f, size, optimize);
}

//DO NOT USE IF RENDERING TO A FRAME
Image *imageDeserialize(const char *path) {
   return _imageDeserializeEXFile(path, false);
}
Image *imageDeserializeFromBuffer(byte *buffer, long size) {
   return _imageDeserializeEX(buffer, size, false);
}

//optimized for rendering to a frame
Image *imageDeserializeOptimized(const char*path){
   return _imageDeserializeEXFile(path, true);
}
Image *imageDeserializeOptimizedFromBuffer(byte *buffer, long size) {
   return _imageDeserializeEX(buffer, size, true);
}

void imageSerialize(Image *self, const char *path) {
   static int sizeofShort = sizeof(uint16_t)*8;
   int i, y;
   long bPos;
   FILE *out;

   short width = imageGetWidth(self);
   short height = imageGetHeight(self);

   //numpixels + size of short + 2 for typeID times number of linee times 5 planes plus two shorts for imageSize
   int maxBitCount = ((width + sizeofShort + 2)*height)*EGA_IMAGE_PLANES + sizeofShort*2;
   int maxByteCount = minByteCount(maxBitCount);

   BitBuffer *buffer = bitBufferCreate(checkedCalloc(1, maxByteCount), 1);

   bitBufferWriteBits(buffer, sizeofShort, (byte*)&width);
   bitBufferWriteBits(buffer, sizeofShort, (byte*)&height);

   for(i = 0; i < EGA_IMAGE_PLANES; ++i) {
      for(y = 0; y < height; ++y) {
         imageScanLineSerialize(imageGetScanLine(self, y, i), buffer);
      }
   }

   bPos = bitBufferGetPosition(buffer);

   out = fopen(path, "wb");
   fwrite(bitBufferGetData(buffer) , sizeof(char), minByteCount(bPos), out);
   fclose (out);

   bitBufferDestroy(buffer);
}



void imageDestroy(Image *self) {
   int i, j;

   for(i = 0; i < EGA_PLANES + 1; ++i) {
      for(j = 0; j < self->height; ++j) {
         imageScanLineDestroy(self->planes[i][j]);
      }

      checkedFree(self->planes[i]);
   }

   checkedFree(self);
}

void imageSetScanLine(Image *self, short position, byte plane, ImageScanLine *sl) {

   imageScanLineDestroy(self->planes[plane][position]);
   self->planes[plane][position] = sl;

}
ImageScanLine *imageGetScanLine(Image *self, short position, byte plane) {
   return self->planes[plane][position];
}

short imageGetWidth(Image *self) {
   return self->width;
}

short imageGetHeight(Image *self) {
   return self->height;
}

Image *buildCheckerboardImage(int width, int height, int tileWidth, byte color1, byte color2) {
   int x, y, j;
   Image *img = imageCreate(width, height);

   SuperScanLine lines[EGA_IMAGE_PLANES] = {0};

   for(y = 0; y < height; ++y){
      for(x = 0; x < width;++x) {
         byte currentColor =  ((x / tileWidth) + (y/tileWidth)) % 2 ? color1 : color2;

         scanLineSetBit((ScanLine*)&lines[0], x, 1);

         for(j = 0; j < EGA_PLANES; ++j) {
            scanLineSetBit((ScanLine*)&lines[j+1], x, !!(currentColor&(1<<j)));
         }
      }

      for(j = 0; j < EGA_IMAGE_PLANES; ++j) {
         imageSetScanLine(img, y, j, createUncompressedScanLine(width, lines[j].pixels));
      }
   }

   return img;
}