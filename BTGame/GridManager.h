#pragma once

#include "Managers.h"
#include "Entities/Entities.h"

#define TABLE_WIDTH 12
#define TABLE_HEIGHT 8
#define CELL_COUNT (TABLE_WIDTH*TABLE_HEIGHT)
#define GRID_X_POS 224
#define GRID_Y_POS 38
#define GRID_RES_SIZE 32
#define INF ((size_t)-1)

typedef struct GridManager_t GridManager;

GridManager *createGridManager(EntitySystem *system);

typedef struct{
   size_t ID;
   vec(EntityPtr) *entities;
}GridNodePublic;

size_t gridNodeGetScore(GridNodePublic *self);

#define ClosureTPart \
    CLOSURE_RET(size_t) /*return edge*/\
    CLOSURE_NAME(GridProcessNeighbor) \
    CLOSURE_ARGS(GridNodePublic */*current*/, GridNodePublic*/*neighbor*/)
#include "segautils\Closure_Decl.h"

#define ClosureTPart \
    CLOSURE_RET(int) /*return true if solved*/ \
    CLOSURE_NAME(GridProcessCurrent) \
    CLOSURE_ARGS(GridNodePublic*/*current*/)
#include "segautils\Closure_Decl.h"

typedef struct {
   size_t node;
}GridSolutionNode;

#define VectorTPart GridSolutionNode
#include "segautils\Vector_Decl.h"

typedef struct {
   size_t totalCost;
   size_t solutionCell;
   vec(GridSolutionNode) *path;
}GridSolution;

GridSolution gridManagerSolve(GridManager *self, size_t startCell, GridProcessCurrent cFunc, GridProcessNeighbor nFunc);
vec(EntityPtr) *gridManagerEntitiesAt(GridManager *self, size_t index);

static size_t gridIndexFromXY(int x, int y){
   if (x < 0 || x >= TABLE_WIDTH || y < 0 || y >= TABLE_HEIGHT){
      return INF;
   }

   return TABLE_WIDTH * y + x;
}

static void gridXYFromIndex(size_t index, int*x, int*y){
   *x = index % TABLE_WIDTH;
   *y = index / TABLE_WIDTH;
}