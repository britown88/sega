#include "Commands.h"
#include "GridManager.h"
#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "SelectionManager.h"
#include <math.h>

typedef struct {
   Entity *e, *dest;
   GridManager *manager;
   size_t destination;
   vec(Int2) *pList;
} GridMoveData;

static GridMoveData *_gridMoveDataCreate(){
   GridMoveData *out = checkedCalloc(1, sizeof(GridMoveData));
   out->pList = vecCreate(Int2)(NULL);
   return out;
}

static void _gridMoveDataDestroy(GridMoveData *self){
   entityDestroy(self->dest);
   vecDestroy(Int2)(self->pList);
   checkedFree(self);
}

typedef struct{
   GridManager *manager;
   size_t destination;

   size_t lowestHeuristic;
   GridNodePublic *closestNode;
}GridSolvingData;

static size_t _processNeighbor(GridSolvingData *data, GridNodePublic *current, GridNodePublic *neighbor){
   vec(EntityPtr) *entities = gridManagerEntitiesAt(data->manager, neighbor->ID);
   if (entities && !vecIsEmpty(EntityPtr)(entities)){
      return INF;
   }
   return gridNodeGetScore(current) + 1;
}

static GridNodePublic *_processCurrent(GridSolvingData *data, GridNodePublic *current){
   size_t currentScore = gridNodeGetScore(current);
   size_t h;
   int x1 = 0, x2 = 0, y1 = 0, y2 = 0;

   if (gridNodeGetScore(current) == INF){
      //cant get to destination, use closest
      return data->closestNode;
   }

   if (current->ID == data->destination){
      //destination found
      return current;
   }

   //update heuristic
   gridXYFromIndex(current->ID, &x1, &y1);
   gridXYFromIndex(data->destination, &x2, &y2);
   h = abs(x1 - x2) + abs(y1 - y2);
   if (h < data->lowestHeuristic){
      data->lowestHeuristic = h;
      data->closestNode = current;
   }

   return  NULL;
}

static GridSolution solve(GridManager *manager, size_t start, size_t destination){
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   GridSolvingData data = { manager, destination, INF, NULL };

   closureInit(GridProcessCurrent)(&cFunc, &data, (GridProcessCurrentFunc)&_processCurrent, NULL);
   closureInit(GridProcessNeighbor)(&nFunc, &data, (GridProcessNeighborFunc)&_processNeighbor, NULL);

   return gridManagerSolve(manager, start, cFunc, nFunc);
}

static void _updateDestLine(Entity *e, size_t destination, vec(Int2) *pList){
   SizeComponent *sc = entityGet(SizeComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
   int halfGridSize = (int)(GRID_RES_SIZE * 0.5f);

   if (pc){
      x0 = pc->x;
      y0 = pc->y;
   }

   if (sc){
      x0 += (int)(sc->x * 0.5f);
      y0 += (int)(sc->y * 0.5f);
   }

   screenPosFromGridIndex(destination, &x1, &y1);
   x1 += halfGridSize;
   y1 += halfGridSize;

   *vecAt(Int2)(pList, 0) = (Int2) {x0, y0 };
   *vecAt(Int2)(pList, 1) = (Int2){ x1, y1 };
}

static CoroutineStatus _gridMove(GridMoveData *data, bool cancel){
   Entity *e = data->e;
   GridComponent *gc = entityGet(GridComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);   

   GridSolution solution;
   size_t pos, dest = INF;

   _updateDestLine(data->e, data->destination, data->pList);

   if (!gc || !pc){
      //doesnt have the right components, we're done
      return Finished;
   }

   if (entityGet(InterpolationComponent)(e)){
      //we're moving, not done, cant be cancelled!
      return NotFinished;
   }

   pos = gridIndexFromXY(gc->x, gc->y);

   if (cancel || pos == data->destination || data->destination == INF){
      //op was cancelled or we made it... so we're done
      return Finished;
   }

   //wasnt cancelled and still have a ways to go...continue on...
   solution = solve(data->manager, pos, data->destination);
   if (solution.totalCost == 0){
      data->destination = solution.solutionCell;
   }

   if (solution.totalCost > 0){
      int x, y;
      dest = vecBegin(GridSolutionNode)(solution.path)->node;

      COMPONENT_LOCK(GridComponent, newgc, e, {
         gridXYFromIndex(dest, &newgc->x, &newgc->y);
      });

      screenPosFromGridXY(gc->x, gc->y, &x, &y);
      COMPONENT_ADD(e, InterpolationComponent, .destX = x, .destY = y, .time = 0.25);

      entityUpdate(e);
   }

   return NotFinished;
}

static void _createLineTransient(GridMoveData *data){
   data->dest = entityCreate(entityGetSystem(data->e));
   COMPONENT_ADD(data->dest, PolygonComponent, .pList = data->pList, .color = 1, .open = true);
   entityUpdate(data->dest);
   entityLinkSelectionTransient(data->e, data->dest);

   vecPushBack(Int2)(data->pList, &(Int2){0, 0});
   vecPushBack(Int2)(data->pList, &(Int2){0, 0});
}

Coroutine createCommandGridMove(Entity *e, GridManager *manager, int x, int y){
   Coroutine out;
   GridMoveData *data = _gridMoveDataCreate();

   data->e = e;
   data->manager = manager;
   data->destination = gridIndexFromXY(x, y);

   _createLineTransient(data);

   closureInit(Coroutine)(&out, data, (CoroutineFunc)&_gridMove, &_gridMoveDataDestroy);
   return out;
}