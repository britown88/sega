#include "Managers.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "CoreComponents.h"

#include "Lua.h"

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
void _onDestroy(ActorManager *self, Entity *e) {}
void _onUpdate(ActorManager *self, Entity *e) {}

static void _updateActor(ActorManager *self, Entity *e) {

}

void actorManagerUpdate(ActorManager *self) {
   luaActorStepAllScripts(self->view->L);
}