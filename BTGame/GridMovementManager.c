#include "Managers.h"
#include "Entities/Entities.h"
#include "CoreComponents.h"
#include "GridManager.h"
#include "GridSolver.h"
#include "WorldView.h"
#include <math.h>

typedef enum {
   Idle,
   Moving,
   Waiting
} MoveStates;

#pragma pack(push, 1)
typedef struct {
   short destX, destY;
   short lastX, lastY;
   Milliseconds moveTime, moveDelay;
   MoveStates state;
}TGridMovingComponent;
#pragma pack(pop)

#define TComponentT TGridMovingComponent
#include "Entities\ComponentDeclTransient.h"

typedef struct {
   GridManager *manager;
   GridSolver *solver;
   size_t destination;

   float lowestHeuristic;
   GridNodePublic *closestNode;
   Entity *entity;//the entity doing the solution
}GridSolvingData;

static float _singleDistance(GridManager *manager, size_t d0, size_t d1) {
   int x0, y0, x1, y1, dist;

   gridManagerXYFromCellID(manager, d0, &x0, &y0);
   gridManagerXYFromCellID(manager, d1, &x1, &y1);

   dist = abs(x0 - x1) + abs(y0 - y1);

   return dist > 1 ? SQRT2 : dist;
}

static int _distance(GridManager *manager, size_t d0, size_t d1) {
   int x0, y0, x1, y1;

   gridManagerXYFromCellID(manager, d0, &x0, &y0);
   gridManagerXYFromCellID(manager, d1, &x1, &y1);

   return abs(x0 - x1) + abs(y0 - y1);
}

int gridDistance(int x0, int y0, int x1, int y1) {
   return abs(x0 - x1) + abs(y0 - y1);
}

static float _processNeighbor(GridSolvingData *data, GridNodePublic *current, GridNodePublic *neighbor) {
   if (neighbor->e) {
      return INFF;
   }

   return gridNodeGetScore(current) + _singleDistance(data->manager, current->ID, neighbor->ID);
}

static GridNodePublic *_processCurrent(GridSolvingData *data, GridNodePublic *current, bool last) {
   float currentScore = gridNodeGetScore(current);
   float h;
   int x1 = 0, x2 = 0, y1 = 0, y2 = 0;

   if (gridNodeGetScore(current) == INF) {
      //cant get to destination, use closest
      return data->closestNode;
   }

   //update heuristic
   gridManagerXYFromCellID(data->manager, current->ID, &x1, &y1);
   gridManagerXYFromCellID(data->manager, data->destination, &x2, &y2);
   h = (float)abs(x1 - x2) + (float)abs(y1 - y2);

   if (current->ID == data->destination) {
      //destination found
      return current;
   }

   if (h < data->lowestHeuristic) {
      data->lowestHeuristic = h;
      data->closestNode = current;
   }

   if (last) {
      //weve exhausted the search area, use closest
      return data->closestNode;
   }

   return  NULL;
}

static GridSolution solve(GridManager *manager, GridSolver *solver, size_t start, size_t destination) {
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   GridSolvingData data = { manager, solver, destination, INFF, NULL };

   closureInit(GridProcessCurrent)(&cFunc, &data, (GridProcessCurrentFunc)&_processCurrent, NULL);
   closureInit(GridProcessNeighbor)(&nFunc, &data, (GridProcessNeighborFunc)&_processNeighbor, NULL);

   return gridSolverSolve(solver, start, cFunc, nFunc);
}

struct GridMovementManager_t {
   Manager m;
   WorldView *view;
};

ImplManagerVTable(GridMovementManager)

static void _actorComponentUpdate(GridMovementManager *self, Entity *e, ActorComponent *oldAC) {
   TGridMovingComponent *tgc = entityGet(TGridMovingComponent)(e);
   ActorComponent *ac = entityGet(ActorComponent)(e);
   if (tgc) {
      tgc->moveDelay = ac->moveDelay;
      tgc->moveTime = ac->moveTime;
   }
}

static void _registerUpdateDelegate(GridMovementManager *self, EntitySystem *system) {
   ComponentUpdate update;

   closureInit(ComponentUpdate)(&update, self, (ComponentUpdateFunc)&_actorComponentUpdate, NULL);
   compRegisterUpdateDelegate(ActorComponent)(system, update);
}

GridMovementManager *createGridMovementManager(WorldView *view) {
   GridMovementManager *out = checkedCalloc(1, sizeof(GridMovementManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(GridMovementManager);

   _registerUpdateDelegate(out, view->entitySystem);

   return out;
}

void _destroy(GridMovementManager *self) {
   checkedFree(self);
}
void _onDestroy(GridMovementManager *self, Entity *e) {}
void _onUpdate(GridMovementManager *self, Entity *e) {}

static void _cleanupEntity(Entity *e) {
   entityRemove(TGridMovingComponent)(e);

   //make sure our interpolation gets cleaned up
   if (entityGet(InterpolationComponent)(e)) {
      entityRemove(InterpolationComponent)(e);
   }

   //make sure our wait gets cleaned up
   if (entityGet(WaitComponent)(e)) {
      entityRemove(WaitComponent)(e);
   }
}

static void _stepMovement(GridManager *manager, GridSolver *solver, Entity *e, Microseconds overflow) {
   GridComponent *gc = entityGet(GridComponent)(e);
   TGridMovingComponent *tgc = entityGet(TGridMovingComponent)(e);
   int dx = 0, dy = 0;
   size_t posID, destID;
   float range = 0;
   GridSolution solution;

   posID = gridManagerCellIDFromXY(manager, gc->x, gc->y);
   destID = gridManagerCellIDFromXY(manager, tgc->destX, tgc->destY);

   //we're there
   if (destID == INF || _distance(manager, posID, destID) <= range) {
      _cleanupEntity(e);      
      return;
   }

   solution = solve(manager, solver, posID, destID);

   if (solution.totalCost <= 0) {
      _cleanupEntity(e);
      return;
   }
   else if (solution.path) {
      size_t dest = vecBegin(GridSolutionNode)(solution.path)->node;
      size_t lastPos = gridManagerCellIDFromXY(manager, tgc->lastX, tgc->lastY);

      if (dest == posID || dest == lastPos) {
         _cleanupEntity(e);
         return;
      }

      tgc->lastX = gc->x;
      tgc->lastY = gc->y;

      tgc->state = Moving;

      //update our grid position to the new cell
      COMPONENT_LOCK(GridComponent, newgc, e, {
         gridManagerXYFromCellID(manager, dest, &newgc->x, &newgc->y);
      });

      COMPONENT_ADD(e, InterpolationComponent,
         .destX = gc->x * GRID_CELL_SIZE,
         .destY = gc->y * GRID_CELL_SIZE,
         .time = tgc->moveTime,
         .overflow = overflow);

      entityUpdate(e);
   }
}

static void _updateGridMovement(GridMovementManager *self, Entity *e) {
   //GridComponent *gc = entityGet(GridComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
   TGridMovingComponent *tgc = entityGet(TGridMovingComponent)(e);
   Microseconds overflow = 0;
   WaitComponent *wc = entityGet(WaitComponent)(e);

   switch (tgc->state) {
   case Idle:
      break;
   case Moving:
      if (!ic || ic->overflow != 0) {

         //not moving or we're done moving
         if (tgc->moveDelay <= 0) {
            //no wait so go straight into move
            tgc->state = Idle;
            overflow = ic ? ic->overflow : 0;
            entityRemove(InterpolationComponent)(e);
            entityUpdate(e);
            _stepMovement(self->view->managers->gridManager, self->view->gridSolver, e, overflow);
         }
         else {
            //time to wait
            tgc->state = Waiting;
            COMPONENT_ADD(e, WaitComponent,
               .time = tgc->moveDelay,
               .overflow = ic ? ic->overflow : 0);
            entityRemove(InterpolationComponent)(e);
            entityUpdate(e);
         }
      }
      break;
   case Waiting:
      if (tgc->moveDelay <= 0 || !wc || wc->overflow != 0) {
         //either there is no movedelay or our wait is finished
         tgc->state = Idle;
         overflow = wc ? wc->overflow : 0;
         entityRemove(WaitComponent)(e);
         entityUpdate(e);
         _stepMovement(self->view->managers->gridManager, self->view->gridSolver, e, overflow);
      }
      break;
   }   
}

void gridMovementManagerUpdate(GridMovementManager *self) {
   COMPONENT_QUERY(self->view->entitySystem, TGridMovingComponent, tgc, {
      _updateGridMovement(self, componentGetParent(tgc, self->view->entitySystem));
   });
}

void gridMovementManagerStopEntity(GridMovementManager *self, Entity *e) {
   TGridMovingComponent *tgc = entityGet(TGridMovingComponent)(e);
   GridComponent *gc = entityGet(GridComponent)(e);

   if (tgc && gc) {
      tgc->destX = gc->x;
      tgc->destY = gc->y;
   }
}

void gridMovementManagerMoveEntity(GridMovementManager *self, Entity *e, short x, short y) {
   TGridMovingComponent *tgc = entityGet(TGridMovingComponent)(e);
   short w = gridManagerWidth(self->view->managers->gridManager);
   short h = gridManagerHeight(self->view->managers->gridManager);
   x = MAX(0, MIN(w - 1, x));
   y = MAX(0, MIN(h - 1, y));

   if (tgc) {
      InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
      WaitComponent *wc = entityGet(WaitComponent)(e);

      tgc->destX = x;
      tgc->destY = y;
      tgc->lastX = tgc->lastY = 0;
      //make sure our interpolation gets cleaned up
      if (ic) { ic->time = tgc->moveTime; }
      if (wc) { wc->time = tgc->moveDelay;  }

   }
   else {
      Milliseconds moveTime = DEFAULT_MOVE_SPEED;
      Milliseconds waitTime = DEFAULT_MOVE_DELAY;
      ActorComponent *ac = entityGet(ActorComponent)(e);

      if (ac) {
         moveTime = ac->moveTime;
         waitTime = ac->moveDelay;
      }

      COMPONENT_ADD(e, TGridMovingComponent,
         .destX = x, .destY = y,
         .lastX = 0, .lastY = 0,
         .moveTime = moveTime, .moveDelay = waitTime,
         .state = Idle);

      _stepMovement(self->view->managers->gridManager, self->view->gridSolver, e, 0);
      
   }
}

void gridMovementManagerMoveEntityRelative(GridMovementManager *self, Entity *e, short x, short y) {
   GridComponent *gc = entityGet(GridComponent)(e);

   gridMovementManagerMoveEntity(self, e, gc->x + x, gc->y + y);
}

bool gridMovementManagerEntityIsMoving(GridMovementManager *self, Entity *e) {
   TGridMovingComponent *tgc = entityGet(TGridMovingComponent)(e);
   return tgc ? true : false;
}

//if an entity is moving, this returns the cell ID they were in befofre their last movement
//returns INF if not moving, useful for drawing
size_t gridMovementManagerGetEntityLastPosition(GridMovementManager *self, Entity *e) {
   TGridMovingComponent *tgc = entityGet(TGridMovingComponent)(e);
   if (tgc) {
      return gridManagerCellIDFromXY(self->view->managers->gridManager, tgc->lastX, tgc->lastY);
   }

   return INF;
}