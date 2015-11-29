#pragma once

#include "segautils/Defs.h"
#include "segautils/Rect.h"
#include "Entities/Entities.h"

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

//collision flags
#define GRID_SOLID_TOP (1 << 0)
#define GRID_SOLID_LEFT (1 << 1)
#define GRID_SOLID_BOTTOM (1 << 2)
#define GRID_SOLID_RIGHT (1 << 3)
#define GRID_SOLID (GRID_SOLID_TOP|GRID_SOLID_LEFT|GRID_SOLID_BOTTOM|GRID_SOLID_RIGHT)

#pragma pack(push, 1)
typedef struct {
   byte schema;
   byte collision;//use "solid" flags
}Tile;
#pragma pack(pop)

typedef struct OcclusionCell_t OcclusionCell;

//returns pointer to the entity array that contains all gridded entities currently in view
vec(EntityPtr) *gridManagerQueryEntities(GridManager *self);
void gridManagerQueryEntitiesRect(GridManager *self, Recti area, vec(EntityPtr) *outlist);

GridManager *createGridManager(WorldView *view);
void gridManagerRender(GridManager *self, Frame *frame);
void gridManagerRenderLighting(GridManager *self, Frame *frame);
void gridManagerSetAmbientLight(GridManager *self, byte level);

short gridManagerWidth(GridManager *self);
short gridManagerHeight(GridManager *self);

//take an area of relative to the lightgrid and a preallocated cell array (minimum size area.width * area.height)
//fills in grid occlusion levels, returns number of occluders found
int gridManagerQueryOcclusion(GridManager *self, Recti *area, OcclusionCell *grid);

size_t gridManagerCellIDFromXY(GridManager *self, int x, int y);
void gridManagerXYFromCellID(GridManager *self, size_t ID, int *x, int *y);
Tile *gridManagerTileAt(GridManager *self, size_t index);
Tile *gridManagerTileAtXY(GridManager *self, int x, int y);


