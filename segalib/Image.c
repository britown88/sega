#include "EGA.h"
#include "bt-utils\BitBuffer.h"
#include <malloc.h>
#include "bt-utils\CheckedMemory.h"
#include "bt-utils\BitTwiddling.h"

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


Image *imageDeserialize(const char *path) 
{
   byte readBuffer[MAX_IMAGE_WIDTH] = {0};
   long size;
   BitBuffer *buffer = bitBufferCreate(readFullFile(path, &size), 1);
   short width, height;
   int y, i;
   Image *img;
   
   width = bitBufferReadShort(buffer);
   height = bitBufferReadShort(buffer);

   img = imageCreate(width, height);

   for(i = 0; i < EGA_IMAGE_PLANES; ++i) {
      for(y = 0; y < height; ++y){
         ImageScanLine *scanLine = NULL;
         byte typeID = 0;
         bitBufferReadBits(buffer, &typeID, 2);

         switch(typeID) {
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

   bitBufferDestroy(buffer);
   return img;
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