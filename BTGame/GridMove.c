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
   Action *a;

   float lowestHeuristic;
   GridNodePublic *closestNode;
}GridSolvingData;

static float _distance(size_t d0, size_t d1){
   int x0, y0, x1, y1, dist;

   gridXYFromIndex(d0, &x0, &y0);
   gridXYFromIndex(d1, &x1, &y1);

   dist = abs(x0 - x1) + abs(y0 - y1);

   return dist > 1 ? SQRT2 : dist;
}

static float _processNeighbor(GridSolvingData *data, GridNodePublic *current, GridNodePublic *neighbor){
   vec(EntityPtr) *entities = gridManagerEntitiesAt(data->manager, neighbor->ID);
   if (entities && !vecIsEmpty(EntityPtr)(entities)){
      return INFF;
   }
   return gridNodeGetScore(current) + _distance(current->ID, neighbor->ID);
}

static GridNodePublic *_processCurrent(GridSolvingData *data, GridNodePublic *current){
   float currentScore = gridNodeGetScore(current);
   float h;
   int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
   RangeComponent *rc = entityGet(RangeComponent)(data->a);

   if (gridNodeGetScore(current) == INF){
      //cant get to destination, use closest
      return data->closestNode;
   }

   //update heuristic
   gridXYFromIndex(current->ID, &x1, &y1);
   gridXYFromIndex(data->destination, &x2, &y2);
   h = (float)abs(x1 - x2) + (float)abs(y1 - y2);

   if (current->ID == data->destination || (rc && h <= rc->range)){
      //destination found
      return current;
   }

   
   if (h < data->lowestHeuristic){
      data->lowestHeuristic = h;
      data->closestNode = current;
   }

   return  NULL;
}

static GridSolution solve(GridManager *manager, Action *a, size_t start, size_t destination){
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   GridSolvingData data = { manager, destination, a, INFF, NULL };

   closureInit(GridProcessCurrent)(&cFunc, &data, (GridProcessCurrentFunc)&_processCurrent, NULL);
   closureInit(GridProcessNeighbor)(&nFunc, &data, (GridProcessNeighborFunc)&_processNeighbor, NULL);

   return gridManagerSolve(manager, start, cFunc, nFunc);
}

static CoroutineStatus _gridMove(GridMoveData *data, bool cancel){
   UserComponent *user = entityGet(UserComponent)(data->a);
   TargetPositionComponent *targetPos = entityGet(TargetPositionComponent)(data->a);
   TargetComponent *targetEntity = entityGet(TargetComponent)(data->a);
   Entity *e = user->user;
   GridComponent *gc = entityGet(GridComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);   

   GridSolution solution;
   size_t pos, dest = INF;
   size_t destination;

   if (!gc || !pc){
      //doesnt have the right components, we're done
      return Finished;
   }   

   if (targetPos){
      destination = gridIndexFromXY(targetPos->x, targetPos->y);
   }
   else if(targetEntity){
      GridComponent *targetGridPos = entityGet(GridComponent)(targetEntity->target);
      if (targetGridPos){
         destination = gridIndexFromXY(targetGridPos->x, targetGridPos->y);
      }
      else{
         //target entity isnt on the grid!
         return Finished;
      }
   }
   else{
      //doesnt have a target, we're done
      return Finished;
   }

   if (entityGet(InterpolationComponent)(e)){
      //we're moving, not done, cant be cancelled!
      return NotFinished;
   }
   
   pos = gridIndexFromXY(gc->x, gc->y);

   if (cancel || pos == destination || destination == INF){
      //op was cancelled or we made it... so we're done
      return Finished;
   }

   //wasnt cancelled and still have a ways to go...continue on...
   solution = solve(data->manager, data->a, pos, destination);

   if (solution.totalCost > 0){
      int x, y;
      double timeToMove;
      dest = vecBegin(GridSolutionNode)(solution.path)->node;

      COMPONENT_LOCK(GridComponent, newgc, e, {
         gridXYFromIndex(dest, &newgc->x, &newgc->y);
      });

      screenPosFromGridXY(gc->x, gc->y, &x, &y);

      timeToMove = 0.25 * (double)_distance(pos, dest);

      COMPONENT_ADD(e, InterpolationComponent, .destX = x, .destY = y, .time = timeToMove);

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

Action *createActionGridPosition(CommandManager *self, int x, int y){
   Action *a = commandManagerCreateAction(self);
   COMPONENT_ADD(a, TargetPositionComponent, .x = x, .y = y);
   entityUpdate(a);

   return a;
}

Action *createActionGridTarget(CommandManager *self, Entity *e){
   Action *a = commandManagerCreateAction(self);
   COMPONENT_ADD(a, TargetComponent, e);
   COMPONENT_ADD(a, RangeComponent, 1.0f);
   entityUpdate(a);

   return a;
}