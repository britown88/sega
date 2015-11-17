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

typedef struct OcclusionCell_t OcclusionCell;

//returns pointer to the entity array that contains all gridded entities currently in view
vec(EntityPtr) *gridManagerQueryEntities(GridManager *self);

GridManager *createGridManager(WorldView *view);
void gridManagerRender(GridManager *self, Frame *frame);
void gridManagerRenderLighting(GridManager *self, Frame *frame);
void gridManagerSetAmbientLight(GridManager *self, byte level);

short gridManagerWidth(GridManager *self);
short gridManagerHeight(GridManager *self);

//changes the schema of a given tile (world-tile coords)
void gridManagerSetTileSchema(GridManager *self, int x, int y, byte schema);

//take an area of relative to the lightgrid and a preallocated cell array (minimum size area.width * area.height)
//fills in grid occlusion levels, returns number of occluders found
int gridManagerQueryOcclusion(GridManager *self, Recti *area, OcclusionCell *grid);

size_t gridManagerCellIDFromXY(GridManager *self, int x, int y);
void gridManagerXYFromCellID(GridManager *self, size_t ID, int *x, int *y);


//TIME FOR SOME GRIDSOLVING

//this is the public-facing grid data, ID is y*width+x, 
//collision holds 4-directional collisiondata
typedef struct {
   size_t ID;
   byte collision;
}GridNodePublic;

//get the score of a gidnode (only known by gridmanager.c)
float gridNodeGetScore(GridNodePublic *self);

#define ClosureTPart \
    CLOSURE_RET(float) /*return edge*/\
    CLOSURE_NAME(GridProcessNeighbor) \
    CLOSURE_ARGS(GridNodePublic */*current*/, GridNodePublic*/*neighbor*/)
#include "segautils\Closure_Decl.h"

#define ClosureTPart \
    CLOSURE_RET(GridNodePublic *) /*return the solution node*/ \
    CLOSURE_NAME(GridProcessCurrent) \
    CLOSURE_ARGS(GridNodePublic*/*current*/)
#include "segautils\Closure_Decl.h"

typedef struct {
   size_t node;//y*width+x
}GridSolutionNode;

#define VectorTPart GridSolutionNode
#include "segautils\Vector_Decl.h"

typedef struct {
   float totalCost;
   size_t solutionCell;
   vec(GridSolutionNode) *path;
}GridSolution;

GridSolution gridManagerSolve(GridManager *self, size_t startCell, GridProcessCurrent cFunc, GridProcessNeighbor nFunc);
