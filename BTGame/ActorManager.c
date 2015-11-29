#include "Managers.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "CoreComponents.h"

#include "Lua.h"

typedef struct {
   EMPTY_STRUCT;
}TActorComponent;

#define TComponentT TActorComponent
#include "Entities\ComponentDeclTransient.h"

struct ActorManager_t {
   Manager m;
   WorldView *view;
};

ImplManagerVTable(ActorManager)

ActorManager *createActorManager(WorldView *view) {
   ActorManager *out = checkedCalloc(1, sizeof(ActorManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(ActorManager);
   return out;
}

void _destroy(ActorManager *self) {
   checkedFree(self);
}
void _onDestroy(ActorManager *self, Entity *e) {
   if (entityGet(TActorComponent)(e)) {
      luaActorRemoveActor(self->view->L, e);
   }
}
void _onUpdate(ActorManager *self, Entity *e) {
   TActorComponent *tac = entityGet(TActorComponent)(e);
   ActorComponent *ac = entityGet(ActorComponent)(e);

   if (ac) {
      if (tac) {
         //already has one
      }
      else {
         //new entry, add a transient
         COMPONENT_ADD(e, TActorComponent, 0);
         luaActorAddActor(self->view->L, e);
      }
   }
   else {
      if (tac) {
         //interpolation comp was removed somehow, cleanup the transient
         entityRemove(TActorComponent)(e);
         luaActorRemoveActor(self->view->L, e);
      }
   }
}

static void _updateActor(ActorManager *self, Entity *e) {

}

void actorManagerUpdate(ActorManager *self) {
   luaActorStepAllScripts(self->view, self->view->L);
}