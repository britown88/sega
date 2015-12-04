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

   bool errorTripped;
};

ImplManagerVTable(ActorManager)

static void _actorComponentUpdate(ActorManager *self, Entity *e, ActorComponent *oldAC) {

}

static void _registerUpdateDelegate(ActorManager *self, EntitySystem *system) {
   ComponentUpdate update;

   closureInit(ComponentUpdate)(&update, self, (ComponentUpdateFunc)&_actorComponentUpdate, NULL);
   compRegisterUpdateDelegate(ActorComponent)(system, update);
}


ActorManager *createActorManager(WorldView *view) {
   ActorManager *out = checkedCalloc(1, sizeof(ActorManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(ActorManager);

   _registerUpdateDelegate(out, view->entitySystem);

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
   if (self->errorTripped) {
      return;
   }

   if (luaActorStepAllScripts(self->view, self->view->L)) {
      consolePrintLuaError(self->view->console, "Error stepping scripts, halting execution of scripts by frame.");
      self->errorTripped = true;
   }
}

void actorManagerClearErrorFlag(ActorManager *self) {
   self->errorTripped = false;
}