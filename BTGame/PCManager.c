#include "Managers.h"
#include "WorldView.h"
#include "LightGrid.h"
#include "segashared/CheckedMemory.h"
#include "ImageLibrary.h"
#include "Lua.h"
#include "Actors.h"


struct PCManager_t {
   WorldView *view;
   Actor *pc;

   bool usingTorch;
   bool sneaking;
};



PCManager *pcManagerCreate(WorldView *view) {
   PCManager *out = checkedCalloc(1, sizeof(PCManager));
   out->view = view;
   return out;
}

void pcManagerDestroy(PCManager *self) {
   actorDestroy(self->pc);
   checkedFree(self);
}

static void _updateSprite(PCManager *self) {

   if (self->sneaking) {
      actorSetImagePos(self->pc, (Int2) {28, 28});
   }
   else if (self->usingTorch) {
      actorSetImagePos(self->pc, (Int2) { 56, 28 });
   }
   else {
      actorSetImagePos(self->pc, (Int2) { 42, 28 });
   }
}

static void _updateLight(PCManager *self) {
   LightSource *ls = actorGetLightSource(self->pc);
   LightSourceParams *light = lightSourceParams(ls);

   if (self->usingTorch && !self->sneaking) {
      light->centerLevel = MAX_BRIGHTNESS;
      light->radius = 0;
      light->fadeWidth = MAX_BRIGHTNESS;
   }
   else {
      light->centerLevel = 2;
      light->radius = 0;
      light->fadeWidth = 2;
   }
}

void pcManagerUpdate(PCManager *self) {
   Viewport *vp = self->view->viewport;
   Int2 aPos = actorGetWorldPosition(self->pc);
   int gridWidth = gridManagerWidth(self->view->gridManager) * GRID_CELL_SIZE;
   int gridHeight = gridManagerHeight(self->view->gridManager) * GRID_CELL_SIZE;
   int xCenter = (GRID_WIDTH / 2) * GRID_CELL_SIZE;
   int yCenter = (GRID_HEIGHT / 2) * GRID_CELL_SIZE;
   int xOffset = MIN(gridWidth - (vp->region.width), MAX(0, aPos.x - xCenter));
   int yOffset = MIN(gridHeight - (vp->region.height), MAX(0, aPos.y - yCenter));

   vp->worldPos = (Int2) { xOffset, yOffset };
}

void pcManagerCreatePC(PCManager *self) {
   self->pc = actorManagerCreateActor(self->view->actorManager);

   actorSetImage(self->pc, stringIntern(IMG_TILE_ATLAS));

   _updateLight(self);
   _updateSprite(self);

   luaActorMakeActorGlobal(self->view->L, self->pc, LLIB_PLAYER);
}

void pcManagerStop(PCManager *self) {
   actorStop(self->pc);
}

void pcManagerMove(PCManager *self, short x, short y) {
   actorMove(self->pc, x, y);
}

void pcManagerMoveRelative(PCManager *self, short x, short y) {
   actorMoveRelative(self->pc, x, y);
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