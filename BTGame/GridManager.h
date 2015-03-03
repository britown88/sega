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
void gridManagerUpdate(GridManager *self);

typedef void* GridSolveData;
typedef size_t(*GridProcessNeighborFunc)(GridManager* /*self*/, GridSolveData/*data*/, size_t/*current*/, size_t/*neighbor*/);//return edge
typedef int(*GridProcessCurrentFunc)(GridManager* /*self*/, GridSolveData/*data*/, size_t/*current*/);//return true if solved

typedef struct {
   size_t totalCost;
   size_t solutionCell;
   size_t nextCell;
}GridSolution;

GridSolution gridManagerSolve(GridManager *self, GridSolveData data, size_t startCell, GridProcessCurrentFunc cFunc, GridProcessNeighborFunc nFunc);
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