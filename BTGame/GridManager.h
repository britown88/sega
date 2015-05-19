#pragma once

#include "Managers.h"
#include "Entities/Entities.h"

#define TABLE_WIDTH 12
#define TABLE_HEIGHT 8
#define CELL_COUNT (TABLE_WIDTH*TABLE_HEIGHT)
#define GRID_X_POS 224
#define GRID_Y_POS 38
#define GRID_RES_SIZE 32

typedef struct GridManager_t GridManager;
typedef struct WorldView_t WorldView;

GridManager *createGridManager(WorldView *view);

typedef struct{
   size_t ID;
   vec(EntityPtr) *entities;
}GridNodePublic;

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
   size_t node;
}GridSolutionNode;

#define VectorTPart GridSolutionNode
#include "segautils\Vector_Decl.h"

typedef struct {
   float totalCost;
   size_t solutionCell;
   vec(GridSolutionNode) *path;
}GridSolution;

GridSolution gridManagerSolve(GridManager *self, size_t startCell, GridProcessCurrent cFunc, GridProcessNeighbor nFunc);
vec(EntityPtr) *gridManagerEntitiesAt(GridManager *self, size_t index);

size_t gridIndexFromScreenXY(int x, int y);
void gridXYFromScreenXY(int x, int y, int *gx, int *gy);
size_t gridIndexFromXY(int x, int y);
void gridXYFromIndex(size_t index, int*x, int*y);
void screenPosFromGridXY(int gx, int gy, int *x, int *y);
void screenPosFromGridIndex(size_t index, int *x, int *y);

int gridDistance(Entity *user, Entity *target);

