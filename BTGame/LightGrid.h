#pragma once
#include "segautils/Defs.h"
#include "GridManager.h"

#define LIGHT_LEVEL_COUNT 7
#define MAX_BRIGHTNESS (LIGHT_LEVEL_COUNT - 1)

#define LIGHT_GRID_WIDTH (GRID_WIDTH + 1)
#define LIGHT_GRID_HEIGHT (GRID_HEIGHT + 1)
#define LIGHT_GRID_CELL_COUNT (LIGHT_GRID_WIDTH * LIGHT_GRID_HEIGHT)

#define LIGHTFLAGS_DIRECTLYLIT 1 << 0
#define LIGHTFLAGS_FILTERED 1 << 1

typedef struct {
   byte level;
   byte flags;
}LightData;

typedef struct OcclusionCell_t {
   byte level;
   int x, y;
   Recti area;
}OcclusionCell;

typedef struct LightSourceParams_t{
   byte radius;
   byte centerLevel;
   byte fadeWidth; // number of tiles devoted to fading out... actual radius will be adjusted toa  minimum of this
   bool on;
}LightSourceParams;

typedef struct LightSource_t LightSource;
LightSourceParams *lightSourceParams(LightSource *self);
Int2 *lightSourcePosition(LightSource *self);//world positions!
void lightSourceDestroy(LightSource *self);//lightsources are registered inside the grid so make sure to free these

typedef struct LightGrid_t LightGrid;
typedef struct Frame_t Frame;
typedef struct FrameRegion_t FrameRegion;

LightGrid *lightGridCreate(GridManager *parent);
void lightGridDestroy(LightGrid *self);

LightSource *lightGridCreateLightSource(LightGrid *self);

//clears the light levels and recalculates
void lightGridUpdate(LightGrid *self, short vpx, short vpy);

void lightGridSetAmbientLight(LightGrid *self, byte level);

//returns null if out of bounds
LightData *lightGridAt(LightGrid *self, byte x, byte y);

//render a given lightData tile at a given x,y (grid-agnostic!)
void lightDataRender(LightData *light, Frame *frame, FrameRegion *vp, short x, short y);


