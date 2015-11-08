#include "Managers.h"
#include "Entities/Entities.h"
#include "CoreComponents.h"
#include "GridManager.h"
#include "WorldView.h"

#pragma pack(push, 1)
typedef struct {
   short destX, destY;
   short nextX, nextY;
}TGridMovingComponent;
#pragma pack(pop)

#define TComponentT TGridMovingComponent
#include "Entities\ComponentDeclTransient.h"

struct GridMovementManager_t {
   Manager m;
   WorldView *view;
};

ImplManagerVTable(GridMovementManager)

GridMovementManager *createGridMovementManager(WorldView *view) {
   GridMovementManager *out = checkedCalloc(1, sizeof(GridMovementManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(GridMovementManager);
   return out;
}

void _destroy(GridMovementManager *self) {
   checkedFree(self);
}
void _onDestroy(GridMovementManager *self, Entity *e) {}
void _onUpdate(GridMovementManager *self, Entity *e) {}

static void _stepMovement(Entity *e) {
   GridComponent *gc = entityGet(GridComponent)(e);
   TGridMovingComponent *tgc = entityGet(TGridMovingComponent)(e);
   int dx = 0, dy = 0;

   //we're there
   if (gc->x == tgc->destX && gc->y == tgc->destY) {
      entityRemove(TGridMovingComponent)(e);
      return;
   }

   if (gc->x != tgc->destX) {
      dx = gc->x < tgc->destX ? 1 : -1;
   }
   else {
      dy = gc->y < tgc->destY ? 1 : -1;
   }

   tgc->nextX = gc->x + dx;
   tgc->nextY = gc->y + dy;

   COMPONENT_ADD(e, InterpolationComponent, 
      .destX = tgc->nextX * GRID_CELL_SIZE,
      .destY = tgc->nextY * GRID_CELL_SIZE,
      .time = 250);

   entityUpdate(e);
}

static void _updateGridMovement(GridMovementManager *self, Entity *e) {
   //GridComponent *gc = entityGet(GridComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
   TGridMovingComponent *tgc = entityGet(TGridMovingComponent)(e);

   if (ic) {
      //still moving
      return;
   }

   COMPONENT_LOCK(GridComponent, gc, e, {
      gc->x = tgc->nextX;
      gc->y = tgc->nextY;
   });

   _stepMovement(e);
}

void gridMovementManagerUpdate(GridMovementManager *self) {
   COMPONENT_QUERY(self->view->entitySystem, TGridMovingComponent, tgc, {
      _updateGridMovement(self, componentGetParent(tgc, self->view->entitySystem));
   });
}

void gridMovementManagerMoveEntity(GridMovementManager *self, Entity *e, short x, short y) {
   TGridMovingComponent *tgc = entityGet(TGridMovingComponent)(e);

   if (tgc) {
      tgc->destX = x;
      tgc->destY = y;
   }
   else {
      COMPONENT_ADD(e, TGridMovingComponent, x, y, 0, 0);
      _stepMovement(e);
      
   }
}