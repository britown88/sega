#include "Managers.h"
#include "segashared/CheckedMemory.h"
#include "Console.h"
#include "Actors.h"
#include "ImageLibrary.h"
#include "GridManager.h"
#include "LightGrid.h"

#include "Lua.h"

#define VectorTPart ActorPtr
#include "segautils/Vector_Impl.h"


struct ActorManager_t {
   WorldView *view;

   bool errorTripped;

   vec(ActorPtr) *actors;
};

struct Actor_t {
   ActorManager *parent;
   Int2 pos, offset;
   Milliseconds moveTime;
   Milliseconds moveDelay;
   ManagedImage *img;   
   Int2 imgPos;
   GridToken *gridToken;
   LightSource *lightSource;
};

Actor *actorManagerCreateActor(ActorManager *self) {
   Actor *a = checkedCalloc(1, sizeof(Actor));

   a->parent = self;
   a->gridToken = gridManagerCreateToken(self->view->gridManager, a);
   a->lightSource = gridManagerCreateLightSource(self->view->gridManager);

   luaActorAddActor(self->view->L, a);
   vecPushBack(ActorPtr)(self->actors, &a);
   return a;
}


void actorDestroy(Actor *self) {
   if (self->img) {
      managedImageDestroy(self->img);
   }

   gridTokenDestroy(self->gridToken);
   lightSourceDestroy(self->lightSource);

   luaActorRemoveActor(self->parent->view->L, self);
   vecRemove(ActorPtr)(self->parent->actors, &self);
   checkedFree(self);
}

void actorPtrDestroy(ActorPtr *self) {
   actorDestroy(*self);
}

GridToken *actorGetGridToken(Actor *self) {
   return self->gridToken;
}
LightSource *actorGetLightSource(Actor *self) {
   return self->lightSource;
}

Int2 actorGetGridPosition(Actor *self) {
   return self->pos;
}
void actorSetGridPosition(Actor *self, Int2 pos) {
   gridTokenMove(self->gridToken, pos);
   *lightSourcePosition(self->lightSource) = pos;
   self->pos = pos;
}
void actorSnap(Actor *self) {
   //snapo offset
   self->offset = (Int2) { 0 };
}

Int2 actorGetWorldPosition(Actor *self) {
   return (Int2){
      self->pos.x * GRID_CELL_SIZE + self->offset.x, 
      self->pos.y * GRID_CELL_SIZE + self->offset.y 
   };
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

void actorSetImage(Actor *self, StringView imgId) {
   if (self->img) {
      managedImageDestroy(self->img);      
   }
   self->img = imageLibraryGetImage(self->parent->view->imageLibrary, imgId);
}
void actorSetImagePos(Actor *self, Int2 imgPos) {
   self->imgPos = imgPos;
}

ActorManager *actorManagerCreate(WorldView *view) {
   ActorManager *out = checkedCalloc(1, sizeof(ActorManager));
   out->view = view;
   out->actors = vecCreate(ActorPtr)(NULL);

   return out;
}
void actorManagerDestroy(ActorManager *self) {
   vecDestroy(ActorPtr)(self->actors);
   checkedFree(self);
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

static void _renderActor(ActorManager *self, Frame *f, Actor *a) {
   Viewport *vp = self->view->viewport;
   Int2 aPos = actorGetWorldPosition(a);

   short renderX = aPos.x - vp->worldPos.x;
   short renderY = aPos.y - vp->worldPos.y;


   frameRenderImagePartial(f, &vp->region, renderX, renderY,
      managedImageGetImage(a->img), a->imgPos.x, a->imgPos.y, GRID_CELL_SIZE, GRID_CELL_SIZE);
}

void actorManagerRender(ActorManager *self, Frame *f) {
   vecForEach(ActorPtr, a, gridManagerQueryActors(self->view->gridManager), {
      _renderActor(self, f, *a);
   });
}