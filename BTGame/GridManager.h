#pragma once

#include "segautils/Defs.h"
#include "segautils/Rect.h"
#include "Tiles.h"
#include "Actors.h"

typedef struct Map_t Map;
typedef struct GridManager_t GridManager;
typedef struct Frame_t Frame;
typedef struct FrameRegion_t FrameRegion;
typedef struct WorldView_t WorldView;

#define GRID_POS_X 13
#define GRID_POS_Y 11

#define GRID_CELL_SIZE 14
#define GRID_WIDTH 21
#define GRID_HEIGHT 11

#define GRID_PX_WIDTH (GRID_WIDTH * GRID_CELL_SIZE)
#define GRID_PX_HEIGHT (GRID_HEIGHT * GRID_CELL_SIZE)

//in milliseconds, for grid traversal
#define DEFAULT_MOVE_SPEED 250
#define DEFAULT_MOVE_DELAY 0

typedef struct OcclusionCell_t OcclusionCell;

GridManager *gridManagerCreate(WorldView *view);
void gridManagerDestroy(GridManager *self);
void gridManagerRender(GridManager *self, Frame *frame);
void gridManagerRenderLighting(GridManager *self, Frame *frame);
void gridManagerSetAmbientLight(GridManager *self, byte level);

Map *gridManagerGetMap(GridManager *self);
void gridManagerLoadMap(GridManager *self, Map *map);

void gridManagerClearSchemas(GridManager *self);
TileSchema *gridManagerGetSchema(GridManager *self, size_t index);
size_t gridManagerGetSchemaCount(GridManager *self);
void gridManagerRenderSchema(GridManager *self, size_t index, Frame *frame, FrameRegion *vp, short x, short y);

void gridManagerSnapActor(GridManager *self, Actor *a);

//returns pointer to the entity array that contains all gridded entities currently in view
vec(ActorPtr) *gridManagerQueryActors(GridManager *self);
void gridManagerQueryActorsRect(GridManager *self, Recti area, vec(ActorPtr) *outlist);
Actor *gridManagerActorFromScreenPosition(GridManager *self, Int2 pos);

short gridManagerWidth(GridManager *self);
short gridManagerHeight(GridManager *self);

//take an area of relative to the lightgrid and a preallocated cell array (minimum size area.width * area.height)
//fills in grid occlusion levels, returns number of occluders found
int gridManagerQueryOcclusion(GridManager *self, Recti *area, OcclusionCell *grid);

size_t gridManagerCellIDFromXY(GridManager *self, int x, int y);
void gridManagerXYFromCellID(GridManager *self, size_t ID, int *x, int *y);
Tile *gridManagerTileAt(GridManager *self, size_t index);
Tile *gridManagerTileAtXY(GridManager *self, int x, int y);
Tile *gridManagerTileAtScreenPos(GridManager *self, int x, int y);


