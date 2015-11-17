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

void pcManagerUpdate(PCManager *self) {
   Viewport *vp = self->view->viewport;
   PositionComponent *pc = entityGet(PositionComponent)(self->pc);
   int gridWidth = gridManagerWidth(self->view->managers->gridManager) * GRID_CELL_SIZE;
   int gridHeight = gridManagerHeight(self->view->managers->gridManager) * GRID_CELL_SIZE;
   int xCenter = (GRID_WIDTH / 2) * GRID_CELL_SIZE;
   int yCenter = (GRID_HEIGHT / 2) * GRID_CELL_SIZE;
   int xOffset = MIN(gridWidth, MAX(0, pc->x - xCenter));
   int yOffset = MIN(gridHeight, MAX(0, pc->y - yCenter));


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
   COMPONENT_ADD(self->pc, LightComponent, 0, MAX_BRIGHTNESS);

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