#include "Managers.h"
#include "Entities/Entities.h"
#include "CoreComponents.h"
#include "WorldView.h"
#include "LightGrid.h"
#include "segashared/CheckedMemory.h"


struct PCManager_t {
   Manager m;
   WorldView *view;

   Entity *pc;
   bool usingTorch;
   bool sneaking;
};

ImplManagerVTable(PCManager)

PCManager *createPCManager(WorldView *view) {
   PCManager *out = checkedCalloc(1, sizeof(PCManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(PCManager);
   return out;
}

void _destroy(PCManager *self) {
   checkedFree(self);
}
void _onDestroy(PCManager *self, Entity *e) {}
void _onUpdate(PCManager *self, Entity *e) {}

static void _updateSprite(PCManager *self) {
   ImageComponent *ic = entityGet(ImageComponent)(self->pc);

   if (self->sneaking) {
      ic->x = 28;
      ic->y = 28;
   }
   else if (self->usingTorch) {
      ic->x = 56;
      ic->y = 28;
   }
   else {
      ic->x = 42;
      ic->y = 28;
   }
}

static void _updateLight(PCManager *self) {
   LightComponent *lc = entityGet(LightComponent)(self->pc);
   
   if (self->usingTorch && !self->sneaking) {
      lc->centerLevel = MAX_BRIGHTNESS;
      lc->radius = 0;
      lc->fadeWidth = MAX_BRIGHTNESS;
   }
   else {
      lc->centerLevel = 2;
      lc->radius = 0;
      lc->fadeWidth = 2;
   }

}

void pcManagerUpdate(PCManager *self) {
   Viewport *vp = self->view->viewport;
   PositionComponent *pc = entityGet(PositionComponent)(self->pc);
   int gridWidth = gridManagerWidth(self->view->managers->gridManager) * GRID_CELL_SIZE;
   int gridHeight = gridManagerHeight(self->view->managers->gridManager) * GRID_CELL_SIZE;
   int xCenter = (GRID_WIDTH / 2) * GRID_CELL_SIZE;
   int yCenter = (GRID_HEIGHT / 2) * GRID_CELL_SIZE;
   int xOffset = MIN(gridWidth - (vp->region.width), MAX(0, pc->x - xCenter));
   int yOffset = MIN(gridHeight - (vp->region.height), MAX(0, pc->y - yCenter));

   vp->worldPos = (Int2) { xOffset, yOffset };
}

void pcManagerCreatePC(PCManager *self) {
   self->pc = entityCreate(self->view->entitySystem);
   COMPONENT_ADD(self->pc, PositionComponent, 0, 0);
   COMPONENT_ADD(self->pc, SizeComponent, 14, 14);
   COMPONENT_ADD(self->pc, RectangleComponent, 0);

   COMPONENT_ADD(self->pc, ImageComponent, .filename = stringIntern("assets/img/tiles.ega"), .partial = true, .x = 56, .y = 28, .width = 14, .height = 14);
   COMPONENT_ADD(self->pc, LayerComponent, LayerGrid);
   COMPONENT_ADD(self->pc, InViewComponent, 0);
   COMPONENT_ADD(self->pc, GridComponent, 11, 6);
   COMPONENT_ADD(self->pc, LightComponent, .radius = 0, .centerLevel = 0, .fadeWidth = 0);

   _updateLight(self);
   _updateSprite(self);
   entityUpdate(self->pc);
}

void pcManagerStop(PCManager *self) {
   gridMovementManagerStopEntity(self->view->managers->gridMovementManager, self->pc);
}

void pcManagerMove(PCManager *self, short x, short y) {
   gridMovementManagerMoveEntity(self->view->managers->gridMovementManager, self->pc, x, y);
}

void pcManagerMoveRelative(PCManager *self, short x, short y) {
   gridMovementManagerMoveEntityRelative(self->view->managers->gridMovementManager, self->pc, x, y);
}

void pcManagerToggleTorch(PCManager *self) {
   self->usingTorch = !self->usingTorch;
   _updateLight(self);
   _updateSprite(self);
}
void pcManagerSetTorch(PCManager *self, bool torchOn) {
   self->usingTorch = torchOn;
   _updateLight(self);
   _updateSprite(self);
}
void pcManagerSetSneak(PCManager *self, bool sneaking) {
   self->sneaking = sneaking;
   _updateLight(self);
   _updateSprite(self);
}