#pragma once

#include "Tiles.h"

typedef struct Map_t Map;

Map *mapCreate(short x, short y);
Map *mapLoad(const char *fileName);
void mapResize(Map *self, short x, short y);
void mapDestroy(Map *self);

int mapSave(Map *save, const char *fileName);

short mapWidth(Map *self);
short mapHeight(Map *self);

Tile *mapGetTiles(Map *self);




Tile *mapTileAtXY(Map *self, int x, int y);
Tile *mapTileAt(Map *self, size_t i);