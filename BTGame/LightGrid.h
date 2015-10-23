#pragma once
#include "segautils/Defs.h"
#include "GridManager.h"

#define LIGHT_LEVEL_COUNT 7
#define MAX_BRIGHTNESS (LIGHT_LEVEL_COUNT - 1)

#define LIGHT_GRID_WIDTH (GRID_WIDTH + 1)
#define LIGHT_GRID_HEIGHT (GRID_HEIGHT + 1)
#define LIGHT_GRID_CELL_COUNT (LIGHT_GRID_WIDTH * LIGHT_GRID_HEIGHT)

typedef struct {
   byte level;
}LightData;

typedef struct LightGrid_t LightGrid;
typedef struct Frame_t Frame;
typedef struct FrameRegion_t FrameRegion;
typedef struct EntitySystem_t EntitySystem;

LightGrid *lightGridCreate();
void lightGridDestroy(LightGrid *self);

void lightGridUpdate(LightGrid *self, EntitySystem *es, byte vpx, byte vpy);

LightData *lightGridAt(LightGrid *self, byte x, byte y);
void lightDataRender(LightData *light, Frame *frame, FrameRegion *vp, short x, short y);
