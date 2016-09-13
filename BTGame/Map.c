#include "Map.h"
#include "segashared/CheckedMemory.h"
#include "segautils/BitBuffer.h"
#include "segautils/BitTwiddling.h"
#include "GridManager.h"

#include "segautils/Defs.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#pragma pack(push, 1)

struct Tile_t {
   byte schema;
   byte collAndFlags;//4bits:collision, 4bits: flags
   byte areas;//4bitsvision 4bitslogic
};

#pragma pack(pop)

#define COLL_MASK   0xF
#define FLAGS_MASK 0xF0

byte tileGetCollision(Tile *self) {
   return self->collAndFlags & COLL_MASK;
}
void tileSetCollision(Tile *self, byte col) {
   self->collAndFlags &= (FLAGS_MASK + (col&COLL_MASK));
}

byte tileGetSchema(Tile *self) {
   return self->schema;
}
void tileSetSchema(Tile *self,byte schema) {
   self->schema = schema;
}




struct Map_t {
   short width, height;
   Tile *grid;
};



Tile *mapTileAtXY(Map *self, int x, int y) {
   return self->grid + (y * self->width + x);
}
Tile *mapTileAt(Map *self, size_t i) {
   return self->grid + i;
}
size_t mapTileIndexFromPointer(Map *self, Tile *t) {
   return t - self->grid;
}

Map *mapCreate(short x, short y) {
   Map *out = checkedCalloc(1, sizeof(Map));
   out->grid = checkedCalloc(x*y, sizeof(Tile));
   out->height = y;
   out->width = x;
   return out;
}

Map *mapCopy(Map *self) {
   Map *out = mapCreate(self->width, self->height);
   memcpy(out->grid, self->grid, self->width * self->height * sizeof(Tile));
   return out;
}

void mapCopyInner(Map *dst, Map *src) {
   if (dst->height != src->height || dst->width != src->width) {
      return;
   }
   memcpy(dst->grid, src->grid, dst->width * dst->height * sizeof(Tile));
}

void mapResize(Map *self, short x, short y) {
   Tile *newGrid;
   short iy;
   
   if (x == self->width && y == self->height) {
      return;
   }

   newGrid = checkedCalloc(x*y, sizeof(Tile));

   for (iy = 0; iy < MIN(y, self->height); ++iy) {
      memcpy(
         newGrid + (iy*x), 
         self->grid + (iy*self->width), 
         MIN(x, self->width) * sizeof(Tile)
         );
   }

   checkedFree(self->grid);
   self->grid = newGrid;
   self->height = y;
   self->width = x;
}

Map *mapLoad(const char *fileName) {
   long size;
   byte *file;
   BitBuffer buffer;
   short width, height;
   Map *map = NULL;

   file = readFullFile(fileName, &size);
   if (!file) {
      return NULL;
   }

   buffer = bitBufferCreate(file, 1);
   width = bitBufferReadShort(&buffer);
   height = bitBufferReadShort(&buffer);

   map = mapCreate(width, height);
   bitBufferReadBits(&buffer, (byte*)map->grid, width * height * sizeof(Tile) * 8);

   bitBufferDestroy(&buffer);

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

   BitBuffer buffer = bitBufferCreate(checkedCalloc(1, maxByteCount), 1);

   bitBufferWriteBits(&buffer, sizeofShort, (byte*)&width);
   bitBufferWriteBits(&buffer, sizeofShort, (byte*)&height);

   bitBufferWriteBits(&buffer, width * height * sizeof(Tile) * 8, (byte*)self->grid);

   bPos = buffer.pos;

   out = fopen(fileName, "wb");
   if (!out) {
      bitBufferDestroy(&buffer);
      return 1;
   }

   fwrite(buffer.buffer, sizeof(char), minByteCount(bPos), out);
   fclose(out);

   bitBufferDestroy(&buffer);

   return 0;
}

short mapWidth(Map *self) { return self->width; }
short mapHeight(Map *self) { return self->height; }

Tile *mapGetTiles(Map *self) { return self->grid; }