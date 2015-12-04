#pragma once

#include "Tiles.h"

typedef struct Map_t Map;

Map *mapCreate(short x, short y);
Map *mapLoad(const char *fileName);
void mapDestroy(Map *self);

int mapSave(const char *fileName);

short mapWidth(Map *self);
short mapHeight(Map *self);

Tile *mapGetTiles(Map *self);