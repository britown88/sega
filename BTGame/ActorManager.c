#include "Managers.h"
#include "segashared/CheckedMemory.h"
#include "Console.h"
#include "Actors.h"
#include "ImageLibrary.h"
#include "GridManager.h"
#include "LightGrid.h"
#include "GameClock.h"
#include "GridSolver.h"
#include <stdlib.h>

#include "Lua.h"

#define VectorTPart ActorPtr
#include "segautils/Vector_Impl.h"

struct ActorManager_t {
   WorldView *view;
   bool errorTripped;

   vec(ActorPtr) *actors;
};

typedef enum {
   Idle = 0,
   Moving,
   Waiting
} MoveStates;

struct Actor_t {
   ActorManager *parent;

   //speed and position
   Int2 gridPos, worldPos;
   Milliseconds moveTime, moveDelay;
   
   //draw data
   ManagedImage *img;   
   Int2 imgPos;

   //tokens for otyher managers
   GridToken *gridToken;
   LightSource *lightSource;

   //pathfinding
   Int2 dest, last, interp, interpStart;
   MoveStates state;
   bool ordered;
   Microseconds time;
};

Actor *actorManagerCreateActor(ActorManager *self) {
   Actor *a = checkedCalloc(1, sizeof(Actor));

   a->parent = self;
   a->gridToken = gridManagerCreateToken(self->view->gridManager, a);
   a->lightSource = gridManagerCreateLightSource(self->view->gridManager);

   luaActorAddActor(self->view->L, a);
   vecPushBack(ActorPtr)(self->actors, &a);
   return a;
}

void actorDestroy(Actor *self) {
   if (self->img) {
      managedImageDestroy(self->img);
   }

   gridTokenDestroy(self->gridToken);
   lightSourceDestroy(self->lightSource);

   luaActorRemoveActor(self->parent->view->L, self);
   vecRemove(ActorPtr)(self->parent->actors, &self);
   checkedFree(self);
}

void actorPtrDestroy(ActorPtr *self) {
   actorDestroy(*self);
}

GridToken *actorGetGridToken(Actor *self) {
   return self->gridToken;
}
LightSource *actorGetLightSource(Actor *self) {
   return self->lightSource;
}

Int2 actorGetGridPosition(Actor *self) {
   return self->gridPos;
}
void actorSetGridPosition(Actor *self, Int2 pos) {
   gridTokenMove(self->gridToken, pos);
   *lightSourcePosition(self->lightSource) = pos;
   self->gridPos = pos;
}

Int2 actorGetWorldPosition(Actor *self) {
   return self->worldPos;
}
Milliseconds actorGetMoveTime(Actor *self) {
   return self->moveTime;
}
Milliseconds actorGetMoveDelay(Actor *self) {
   return self->moveDelay;
}
void actorSetMoveTime(Actor *self, Milliseconds time) {
   self->moveTime = time;
}
void actorSetMoveDelay(Actor *self, Milliseconds delay) {
   self->moveDelay = delay;
}

void actorSetImage(Actor *self, StringView imgId) {
   if (self->img) {
      managedImageDestroy(self->img);      
   }
   self->img = imageLibraryGetImage(self->parent->view->imageLibrary, imgId);
}
void actorSetImagePos(Actor *self, Int2 imgPos) {
   self->imgPos = imgPos;
}

ActorManager *actorManagerCreate(WorldView *view) {
   ActorManager *out = checkedCalloc(1, sizeof(ActorManager));
   out->view = view;
   out->actors = vecCreate(ActorPtr)(NULL);

   return out;
}
void actorManagerDestroy(ActorManager *self) {
   vecDestroy(ActorPtr)(self->actors);
   checkedFree(self);
}

static void _updateActorMovement(ActorManager *self, Actor *a);

void actorManagerUpdate(ActorManager *self) {
   //update pathfindin/movement
   vecForEach(ActorPtr, a, self->actors, {
      _updateActorMovement(self, *a);
   });

   //and udpate coroutines
   if (!self->errorTripped && luaActorStepAllScripts(self->view, self->view->L)) {
      consolePrintLuaError(self->view->console, "Error stepping scripts, halting execution of scripts by frame.");
      self->errorTripped = true;
   }
}

void actorManagerClearErrorFlag(ActorManager *self) {
   self->errorTripped = false;
}

static void _renderActor(ActorManager *self, Frame *f, Actor *a) {
   Viewport *vp = self->view->viewport;

   short renderX = a->worldPos.x - vp->worldPos.x;
   short renderY = a->worldPos.y - vp->worldPos.y;

   frameRenderImagePartial(f, &vp->region, renderX, renderY,
      managedImageGetImage(a->img), a->imgPos.x, a->imgPos.y, GRID_CELL_SIZE, GRID_CELL_SIZE);
}

void actorManagerRender(ActorManager *self, Frame *f) {
   vecForEach(ActorPtr, a, gridManagerQueryActors(self->view->gridManager), {
      _renderActor(self, f, *a);
   });
}

void actorSnap(Actor *self) {
   self->worldPos.x = self->gridPos.x * GRID_CELL_SIZE;
   self->worldPos.y = self->gridPos.y * GRID_CELL_SIZE;
}


typedef struct {
   GridManager *manager;
   GridSolver *solver;
   size_t destination;

   float lowestHeuristic;
   GridNodePublic *closestNode;
   Actor *actor;//the actor doing the solution
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

static float _processNeighbor(GridSolvingData *data, GridNodePublic *current, GridNodePublic *neighbor) {
   if (neighbor->a) {
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


static void _cleanupActor(Actor *a) {
   a->ordered = false;
}

static void _stepMovement(GridManager *manager, GridSolver *solver, Actor *a, Microseconds overflow) {
   int dx = 0, dy = 0;
   size_t posID, destID;
   float range = 0;
   GridSolution solution;

   posID = gridManagerCellIDFromXY(manager, a->gridPos.x, a->gridPos.y);
   destID = gridManagerCellIDFromXY(manager, a->dest.x, a->dest.y);

   //we're there
   if (destID == INF || _distance(manager, posID, destID) <= range) {
      _cleanupActor(a);
      return;
   }

   solution = solve(manager, solver, posID, destID);

   if (solution.totalCost <= 0) {
      _cleanupActor(a);
      return;
   }
   else if (solution.path) {
      size_t dest = vecBegin(GridSolutionNode)(solution.path)->node;
      size_t lastPos = gridManagerCellIDFromXY(manager, a->last.x, a->last.y);
      Int2 target = { 0 };

      if (dest == posID || dest == lastPos) {
         _cleanupActor(a);
         return;
      }

      a->last.x = a->gridPos.x;
      a->last.y = a->gridPos.y;

      a->interpStart = a->worldPos;

      gridManagerXYFromCellID(manager, dest, &target.x, &target.y);
      actorSetGridPosition(a, target);

      a->interp.x = a->gridPos.x * GRID_CELL_SIZE;
      a->interp.y = a->gridPos.y * GRID_CELL_SIZE;

      a->state = Moving;
      a->time = gameClockGetTime(gameClockGet()) - overflow;
   }
}



void _updateActorMovement(ActorManager *self, Actor *a) {
   WorldView *view = self->view;
   Microseconds time = gameClockGetTime(view->gameClock);
   Microseconds overflow = 0;

   if (!a->ordered) {
      return;
   }

   switch (a->state) {
   case Idle:
      break;
   case Moving: {
      Microseconds delta = time - a->time;
      Microseconds target = t_m2u(a->moveTime);
      bool done = false;

      double m = delta / (double)target;
      if (m > 1.0) {
         m = 1.0;
         overflow = delta - target;
         done = true;
      }

      a->worldPos.x = (int)((a->interp.x - a->interpStart.x) * m) + a->interpStart.x;
      a->worldPos.y = (int)((a->interp.y - a->interpStart.y) * m) + a->interpStart.y;

      if (done) {
         //not moving or we're done moving
         if (a->moveDelay == 0) {
            //no wait so go straight into move
            a->state = Idle;
            _stepMovement(view->gridManager, view->gridSolver, a, overflow);
         }
         else {
            //time to wait
            a->state = Waiting;
            a->time = time - overflow;
         }
      }

   }  break;
   case Waiting: {
      Microseconds delta = time - a->time;
      Microseconds target = t_m2u(a->moveTime);

      if (delta >= target) {
         //done waiting, onto idle
         overflow = delta - target;
         a->state = Idle;
         _stepMovement(view->gridManager, view->gridSolver, a, overflow);
      }
   }  break;
   }
}

void actorStop(Actor *self) {
   self->dest = self->gridPos;
}
void actorMove(Actor *self, short x, short y) {
   WorldView *view = self->parent->view;
   short w = gridManagerWidth(view->gridManager);
   short h = gridManagerHeight(view->gridManager);
   x = MAX(0, MIN(w - 1, x));
   y = MAX(0, MIN(h - 1, y));

   self->dest.x = x;
   self->dest.y = y;
   self->last = (Int2) { 0 };

   if (!self->ordered) {
      self->state = Idle;
      self->ordered = true;
      _stepMovement(view->gridManager, view->gridSolver, self, 0);
   }
}
void actorMoveRelative(Actor *self, short x, short y) {
   actorMove(self, self->gridPos.x + x, self->gridPos.y + y);
}
bool actorIsMoving(Actor *self) {
   return self->ordered;
}