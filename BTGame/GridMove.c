#include "Commands.h"
#include "GridManager.h"
#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "SelectionManager.h"
#include "Actions.h"
#include <math.h>

typedef struct {
   Action *a;
   GridManager *manager;
} GridMoveData;

static GridMoveData *_gridMoveDataCreate(){
   GridMoveData *out = checkedCalloc(1, sizeof(GridMoveData));
   return out;
}

static void _gridMoveDataDestroy(GridMoveData *self){
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

static CoroutineStatus _gridMove(GridMoveData *data, bool cancel){
   UserComponent *user = entityGet(UserComponent)(data->a);
   TargetPositionComponent *target = entityGet(TargetPositionComponent)(data->a);
   Entity *e = user->user;
   GridComponent *gc = entityGet(GridComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);   

   GridSolution solution;
   size_t pos, dest = INF;
   size_t destination;

   if (!target || !gc || !pc){
      //doesnt have the right components, we're done
      return Finished;
   }

   if (entityGet(InterpolationComponent)(e)){
      //we're moving, not done, cant be cancelled!
      return NotFinished;
   }

   destination = gridIndexFromXY(target->x, target->y);
   pos = gridIndexFromXY(gc->x, gc->y);

   if (cancel || pos == destination || destination == INF){
      //op was cancelled or we made it... so we're done
      return Finished;
   }

   //wasnt cancelled and still have a ways to go...continue on...
   solution = solve(data->manager, pos, destination);
   if (solution.totalCost == 0){
      gridXYFromIndex(solution.solutionCell, &target->x, &target->y);
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

Coroutine createCommandGridMove(Action *a, GridManager *manager){
   Coroutine out;
   GridMoveData *data = _gridMoveDataCreate();

   data->a = a;
   data->manager = manager;

   closureInit(Coroutine)(&out, data, (CoroutineFunc)&_gridMove, &_gridMoveDataDestroy);
   return out;
}

Action *createActionGridMove(CommandManager *self, int x, int y){
   Action *a = commandManagerCreateAction(self);
   COMPONENT_ADD(a, TargetPositionComponent, .x = x, .y = y);
   entityUpdate(a);

   return a;
}