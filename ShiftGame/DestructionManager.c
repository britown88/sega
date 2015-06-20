#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"

struct DestructionManager_t{
   Manager m;
   WorldView *view;

   vec(EntityPtr) *toDestroy;
};

static void  _destroyDestroyed(EntityPtr *e){
   entityDestroy(*e);
}

ImplManagerVTable(DestructionManager)

DestructionManager *createDestructionManager(WorldView *view){
   DestructionManager *out = checkedCalloc(1, sizeof(DestructionManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(DestructionManager);

   out->toDestroy = vecCreate(EntityPtr)(&_destroyDestroyed);

   return out;
}

void _destroy(DestructionManager *self){
   vecDestroy(EntityPtr)(self->toDestroy);

   checkedFree(self);
}
void _onDestroy(DestructionManager *self, Entity *e){}
void _onUpdate(DestructionManager *self, Entity *e){}

void destructionManagerUpdate(DestructionManager *self){
   COMPONENT_QUERY(self->view->entitySystem, DestructionComponent, dc, {
      Entity *e = componentGetParent(dc, self->view->entitySystem);
      vecPushBack(EntityPtr)(self->toDestroy, &e);
   });

   vecClear(EntityPtr)(self->toDestroy);
}