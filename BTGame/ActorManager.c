#include "Managers.h"
#include "segashared/CheckedMemory.h"
#include "Console.h"
#include "Actors.h"

#include "Lua.h"

#define VectorTPart ActorPtr
#include "segautils/Vector_Impl.h"


struct ActorManager_t {
   WorldView *view;

   bool errorTripped;
};

struct Actor_t {
   ActorManager *parent;
   Int2 pos;
   Milliseconds moveTime;
   Milliseconds moveDelay;
};

ActorManager *actorManagerCreate(WorldView *view) {
   ActorManager *out = checkedCalloc(1, sizeof(ActorManager));
   out->view = view;

   return out;
}
void actorManagerDestroy(ActorManager *self) {
   checkedFree(self);
}

void actorManagerRender(ActorManager *self, Frame *f) {

}

Actor *actorManagerCreateActor(ActorManager *self) {
   Actor *a = checkedCalloc(1, sizeof(Actor));
   a->parent = self;
   luaActorAddActor(self->view->L, a);
   return a;
}



void actorDestroy(Actor *self) {
   luaActorRemoveActor(self->parent->view->L, self);
}

Int2 actorGetPosition(Actor *self) {
   return self->pos;
}
void actorSetPosition(Actor *self, Int2 pos) {
   self->pos = pos;
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