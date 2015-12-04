#include "Map.h"
#include "segashared/CheckedMemory.h"
#include "segautils/BitBuffer.h"
#include "segautils/BitTwiddling.h"

#include <stdint.h>
#include <stdio.h>

struct Map_t {
   short width, height;
   Tile *grid;
};

Map *mapCreate(short x, short y) {
   Map *out = checkedCalloc(1, sizeof(Map));
   out->grid = checkedCalloc(x*y, sizeof(Tile));
   out->height = y;
   out->width = x;
   return out;
}
Map *mapLoad(const char *fileName) {
   long size;
   byte *file;
   BitBuffer *buffer;
   short width, height;
   Map *map = NULL;

   file = readFullFile(fileName, &size);
   if (!file) {
      return NULL;
   }

   buffer = bitBufferCreate(file, 1);
   width = bitBufferReadShort(buffer);
   height = bitBufferReadShort(buffer);

   map = mapCreate(width, height);
   bitBufferReadBits(buffer, (byte*)map->grid, width * height * sizeof(Tile) * 8);

   return map;
}
void mapDestroy(Map *self) {
   checkedFree(self->grid);
   checkedFree(self);
}

int mapSave(Map *self, const char *fileName) {
   static int sizeofShort = sizeof(uint16_t) * 8;
   long bPos;
   FILE *out;

   short width = mapWidth(self);
   short height = mapHeight(self);

   int maxBitCount = (sizeofShort*2)+(width*height*sizeof(Tile)*8);
   int maxByteCount = minByteCount(maxBitCount);

   BitBuffer *buffer = bitBufferCreate(checkedCalloc(1, maxByteCount), 1);

   bitBufferWriteBits(buffer, sizeofShort, (byte*)&width);
   bitBufferWriteBits(buffer, sizeofShort, (byte*)&height);

   bitBufferWriteBits(buffer, width * height * sizeof(Tile) * 8, (byte*)self->grid);

   bPos = bitBufferGetPosition(buffer);

   out = fopen(fileName, "wb");
   if (!out) {
      bitBufferDestroy(buffer);
      return 1;
   }

   fwrite(bitBufferGetData(buffer), sizeof(char), minByteCount(bPos), out);
   fclose(out);

   bitBufferDestroy(buffer);

   return 0;
}

short mapWidth(Map *self) { return self->width; }
short mapHeight(Map *self) { return self->height; }

Tile *mapGetTiles(Map *self) { return self->grid; }