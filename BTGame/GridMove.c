#include "Commands.h"
#include "GridManager.h"
#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "SelectionManager.h"
#include "Actions.h"
#include "Combat.h"
#include <math.h>

typedef struct {
   Action *a;
   GridManager *manager;
   bool paused;
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

static float _singleDistance(size_t d0, size_t d1){
   int x0, y0, x1, y1, dist;

   gridXYFromIndex(d0, &x0, &y0);
   gridXYFromIndex(d1, &x1, &y1);

   dist = abs(x0 - x1) + abs(y0 - y1);

   return dist > 1 ? SQRT2 : dist;
}

static int _distance(size_t d0, size_t d1){
   int x0, y0, x1, y1;

   gridXYFromIndex(d0, &x0, &y0);
   gridXYFromIndex(d1, &x1, &y1);

   return abs(x0 - x1) + abs(y0 - y1);
}

static float _processNeighbor(GridSolvingData *data, GridNodePublic *current, GridNodePublic *neighbor){
   vec(EntityPtr) *entities = gridManagerEntitiesAt(data->manager, neighbor->ID);
   if (entities && !vecIsEmpty(EntityPtr)(entities)){
      return INFF;
   }
   return gridNodeGetScore(current) + _singleDistance(current->ID, neighbor->ID);
}

static GridNodePublic *_processCurrent(GridSolvingData *data, GridNodePublic *current){
   float currentScore = gridNodeGetScore(current);
   float h;
   int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
   ActionRangeComponent *rc = entityGet(ActionRangeComponent)(data->a);

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

static CoroutineStatus _gridMove(GridMoveData *data, CoroutineRequest request){
   ActionUserComponent *user = entityGet(ActionUserComponent)(data->a);
   ActionTargetPositionComponent *targetPos = entityGet(ActionTargetPositionComponent)(data->a);
   ActionTargetEntityComponent *targetEntity = entityGet(ActionTargetEntityComponent)(data->a);
   Entity *e = user->user;
   GridComponent *gc = entityGet(GridComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);  
   ActionRangeComponent *rc = entityGet(ActionRangeComponent)(data->a);
   float range = 0;

   GridSolution solution;
   size_t pos, dest = INF;
   size_t destination;

   if (!gc || !pc){
      //doesnt have the right components, we're done
      return Finished;
   }  

   if (request == Pause && !data->paused){
      //ensure proper state for pausing
      data->paused = true;
      if (entityGet(InterpolationComponent)(e)){
         entityRemove(InterpolationComponent)(e);
         entityUpdate(e);
      }
      return NotFinished;
   }

   if (request == ForceCancel){
      //forced cancel, snap to target and end
      InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
      if (ic){
         COMPONENT_LOCK(PositionComponent, ppc, e, {
            ppc->x = ic->destX;
            ppc->y = ic->destY;
         });
         entityRemove(InterpolationComponent)(e);
         entityUpdate(e);
      }
      
      return Finished;
   }

   if (data->paused){
      data->paused = false;
      //resume from pause
   }

   if (entityGet(InterpolationComponent)(e)){
      //we're moving, not done, cant be cancelled!
      return NotFinished;
   }

   if (targetPos){
      destination = gridIndexFromXY(targetPos->x, targetPos->y);
   }
   else if(targetEntity){
      GridComponent *targetGridPos = entityGet(GridComponent)(targetEntity->target);
      if (targetGridPos && !entityIsDead(targetEntity->target)){
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
   
   pos = gridIndexFromXY(gc->x, gc->y);

   if (rc){
      range = rc->range;
   }

   if (requestIsCancel(request) || //cancelled
      _distance(pos, destination) <= range || //at our destination
      destination == INF){//impossible

      //so we're done
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

      timeToMove = 0.25 * (double)_singleDistance(pos, dest);

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

   COMPONENT_ADD(a, ActionTargetPositionComponent, .x = x, .y = y);
   entityUpdate(a);

   return a;
}

Action *createActionGridTarget(CommandManager *self, Entity *e, float range){
   Action *a = commandManagerCreateAction(self);
   COMPONENT_ADD(a, ActionTargetEntityComponent, e);
   COMPONENT_ADD(a, ActionRangeComponent, range);

   entityUpdate(a);

   return a;
}
