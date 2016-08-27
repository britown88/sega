#pragma once
#include "segautils/Defs.h"
#include "GridManager.h"
#include "Viewport.h"

#define LIGHT_LEVEL_COUNT 7
#define MAX_BRIGHTNESS (LIGHT_LEVEL_COUNT - 1)

#define LIGHT_GRID_WIDTH (GRID_WIDTH + 1)
#define LIGHT_GRID_HEIGHT (GRID_HEIGHT + 1)
#define LIGHT_GRID_CELL_COUNT (LIGHT_GRID_WIDTH * LIGHT_GRID_HEIGHT)

#define LIGHTFLAGS_DIRECTLYLIT 1 << 0
#define LIGHTFLAGS_FILTERED 1 << 1



typedef struct LightData_t LightData;
byte lightDataGetLevel(LightData *self);

typedef struct OcclusionCell_t {
   byte level;
   int x, y;//lightgrid coords
   int wx, wy;//world coords
   Recti area;
   byte set;
}OcclusionCell;

typedef struct LightSourceParams_t{
   byte radius;
   byte centerLevel;
   byte fadeWidth; // number of tiles devoted to fading out... actual radius will be adjusted toa  minimum of this
   bool on;
}LightSourceParams;

typedef struct LightSource_t LightSource;
void lightSourceSetParams(LightSource *self, LightSourceParams params);
void lightSourceSetPosition(LightSource *self, Int2 pos);//world positions!
void lightSourceDestroy(LightSource *self);//lightsources are registered inside the grid so make sure to free these

typedef struct LightGrid_t LightGrid;
typedef struct Frame_t Frame;
typedef struct FrameRegion_t FrameRegion;

LightGrid *lightGridCreate(GridManager *parent);
void lightGridDestroy(LightGrid *self);

void lightGridLoadMap(LightGrid *self, int width, int height);
void lightGridChangeTileSchema(LightGrid *self, size_t tile, TileSchema *schema);

LightSource *lightGridCreateLightSource(LightGrid *self);

void lightGridSetAmbientLight(LightGrid *self, byte level);

//clears the light levels and recalculates (give grid position of the viewport)
void lightGridUpdate(LightGrid *self, short vpx, short vpy);

//returns null if out of bounds (x and y are coordinates into the last-updated light grid)
LightData *lightGridAt(LightGrid *self, byte x, byte y);


//render a given lightData tile at a given x,y (pixel coordinates in region)
void lightDataRender(LightData *light, Frame *frame, FrameRegion *vp, short x, short y);
void testLightRender(LightGrid *self, Frame *frame, Viewport *vp);


