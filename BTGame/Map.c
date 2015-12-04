#include "Map.h"
#include "segashared/CheckedMemory.h"

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
   return NULL;
}
void mapDestroy(Map *self) {
   checkedFree(self->grid);
   checkedFree(self);
}

int mapSave(const char *fileName) {
   return 0;
}

short mapWidth(Map *self) { return self->width; }
short mapHeight(Map *self) { return self->height; }

Tile *mapGetTiles(Map *self) { return self->grid; }