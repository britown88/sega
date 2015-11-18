#pragma once
#include "WorldView.h"

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
    CLOSURE_ARGS(GridNodePublic*/*current*/, bool/*lastcheck*/)
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

typedef struct GridSolver_t GridSolver;

GridSolver *gridSolverCreate(WorldView *view);
void gridSolverDestroy(GridSolver *self);

GridSolution gridSolverSolve(GridSolver *self, size_t startCell, GridProcessCurrent cFunc, GridProcessNeighbor nFunc);
