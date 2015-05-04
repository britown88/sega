#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"

struct SelectionManager_t{
   Manager m;
   EntitySystem *system;
   vec(EntityPtr) *cursors, *selectList;
};

ImplManagerVTable(SelectionManager)

SelectionManager *createSelectionManager(EntitySystem *system){
   SelectionManager *out = checkedCalloc(1, sizeof(SelectionManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(SelectionManager);

   out->cursors = vecCreate(EntityPtr)(&entityVectorDestroy);
   out->selectList = vecCreate(EntityPtr)(NULL);
   return out;
}

void _destroy(SelectionManager *self){
   vecDestroy(EntityPtr)(self->cursors);
   vecDestroy(EntityPtr)(self->selectList);
   checkedFree(self);
}
void _onDestroy(SelectionManager *self, Entity *e){}
void _onUpdate(SelectionManager *self, Entity *e){}

static void _checkSelection(SelectionManager *self, Recti box, Entity *e){
   PositionComponent *pc = entityGet(PositionComponent)(e);
   TeamComponent *tc = entityGet(TeamComponent)(e);
   SizeComponent *sc = entityGet(SizeComponent)(e);

   if (pc && tc && sc){
      Recti ebox = { pc->x, pc->y, pc->x + sc->x, pc->y + sc->y };
      if (tc->teamID == 0 && rectiIntersects(box, ebox)){
         vecPushBack(EntityPtr)(self->selectList, &e);         
      }
   }
}

void selectionManagerSelect(SelectionManager *self, Recti box){
   vecClear(EntityPtr)(self->cursors);
   vecClear(EntityPtr)(self->selectList);

   COMPONENT_QUERY(self->system, TeamComponent, tc, {
      Entity *e = componentGetParent(tc, self->system);
      _checkSelection(self, box, e);
   });

   vecForEach(EntityPtr, selected, self->selectList, {
      Entity *cursor = entityCreate(self->system);

      COMPONENT_ADD(cursor, PositionComponent, 0, 0);
      COMPONENT_ADD(cursor, ImageComponent, stringIntern("assets/img/select.ega"));

      COMPONENT_ADD(cursor, LayerComponent, LayerTokens);;
      COMPONENT_ADD(cursor, SizeComponent, 32, 32);
      COMPONENT_ADD(cursor, LockedPositionComponent, *selected);

      entityUpdate(cursor);
      vecPushBack(EntityPtr)(self->cursors, &cursor);
   });

}