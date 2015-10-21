#pragma once

#include "segautils/Defs.h"

typedef struct GridManager_t GridManager;
typedef struct Frame_t Frame;
typedef struct WorldView_t WorldView;

#define GRID_POS_X 13
#define GRID_POS_Y 11

#define GRID_CELL_SIZE 14
#define GRID_WIDTH 21
#define GRID_HEIGHT 11

#define GRID_PX_WIDTH (GRID_WIDTH * GRID_CELL_SIZE)
#define GRID_PX_HEIGHT (GRID_HEIGHT * GRID_CELL_SIZE)

#define LIGHT_LEVEL_COUNT 7
#define MAX_BRIGHTNESS (LIGHT_LEVEL_COUNT - 1)

GridManager *createGridManager(WorldView *view);
void gridManagerRender(GridManager *self, Frame *frame);
