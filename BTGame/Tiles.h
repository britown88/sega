#pragma once

#include "segautils/Defs.h"

//collision flags
#define COL_SOLID_TOP (1 << 0)
#define COL_SOLID_LEFT (1 << 1)
#define COL_SOLID_BOTTOM (1 << 2)
#define COL_SOLID_RIGHT (1 << 3)
#define COL_SOLID (COL_SOLID_TOP|COL_SOLID_LEFT|COL_SOLID_BOTTOM|COL_SOLID_RIGHT)

typedef struct Sprite_t Sprite;

#pragma pack(push, 1)

typedef struct {
   byte schema;
   byte collision;//use "solid" flags
}Tile;

typedef struct {
   Sprite *sprite;
   byte occlusion;

   //lighting
   bool lit;
   byte radius;
   byte centerLevel;
   byte fadeWidth;
}TileSchema;

#pragma pack(pop)
