#pragma once

#include "segautils/Defs.h"
#include "segautils/Rect.h"
#include "Tiles.h"
#include "Actors.h"

typedef struct Map_t Map;
typedef struct GridManager_t GridManager;
typedef struct Texture_t Texture;
typedef struct FrameRegion_t FrameRegion;
typedef struct WorldView_t WorldView;

//defines screen area for grid display
#define GRID_POS_X 13
#define GRID_POS_Y 11

#define _GRID_WIDTH 21
#define _GRID_HEIGHT 11

#define GRID_CELL_SIZE 14
#define GRID_SIZE_X (_GRID_WIDTH * GRID_CELL_SIZE)
#define GRID_SIZE_Y (_GRID_HEIGHT * GRID_CELL_SIZE)




//in milliseconds, for grid traversal
#define DEFAULT_MOVE_SPEED 250
#define DEFAULT_MOVE_DELAY 0

typedef struct OcclusionCell_t OcclusionCell;

typedef struct GridToken_t GridToken;//ties an actor to the grid partition table
void gridTokenDestroy(GridToken *self);
void gridTokenMove(GridToken *self, Int2 newPos);

GridManager *gridManagerCreate(WorldView *view);
void gridManagerDestroy(GridManager *self);

GridToken *gridManagerCreateToken(GridManager *self, Actor *owner);

typedef struct LightSource_t LightSource;
LightSource *gridManagerCreateLightSource(GridManager *self);//the gr5id manager owns the light source so


void gridManagerRender(GridManager *self, Texture *tex);
void gridManagerRenderLighting(GridManager *self, Texture *tex);
void gridManagerSetAmbientLight(GridManager *self, byte level);

Map *gridManagerGetMap(GridManager *self);
void gridManagerLoadMap(GridManager *self, Map *map);

void gridManagerLoadSchemaTable(GridManager *self, const char *set);
TileSchema *gridManagerGetSchema(GridManager *self, size_t index);
size_t gridManagerGetSchemaCount(GridManager *self);
void gridManagerRenderSchema(GridManager *self, size_t index, Texture *tex, FrameRegion *vp, short x, short y);

//returns pointer to the actor array that contains all gridded entities currently in view
vec(ActorPtr) *gridManagerQueryActors(GridManager *self);
void gridManagerQueryActorsRect(GridManager *self, Recti area, vec(ActorPtr) *outlist);
Actor *gridManagerActorFromScreenPosition(GridManager *self, Int2 pos);

short gridManagerWidth(GridManager *self);
short gridManagerHeight(GridManager *self);

//take an area of relative to the lightgrid and a preallocated cell array (minimum size area.width * area.height)
//fills in grid occlusion levels, returns number of occluders found
int gridManagerQueryOcclusion(GridManager *self, Recti *area, OcclusionCell *grid);

size_t gridManagerCellIDFromXY(GridManager *self, int x, int y);
size_t gridManagerCellFromTile(GridManager *self, Tile *t);
void gridManagerXYFromCellID(GridManager *self, size_t ID, int *x, int *y);
Tile *gridManagerTileAt(GridManager *self, size_t index);
Tile *gridManagerTileAtXY(GridManager *self, int x, int y);
Tile *gridManagerTileAtScreenPos(GridManager *self, int x, int y);

//use this or else lighting wont be udpated
void gridManagerChangeTileSchema(GridManager *self, size_t tile, byte schema);
void gridManagerSetTileCollision(GridManager *self, size_t tile, byte coll);

int gridDistance(int x0, int y0, int x1, int y1);

void gridManagerToggleLightMode(GridManager *self);
void gridManagerDebugLights(GridManager *self, Int2 source, Int2 target);//WORLD CGRID COORDS

void gridManagerSaveSnapshot(GridManager *self);
void gridManagerUndo(GridManager *self);
void gridManagerRedo(GridManager *self);


