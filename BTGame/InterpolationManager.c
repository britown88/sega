#include "Managers.h"

#include "Managers.h"
#include "ImageLibrary.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "segautils\Defs.h"

#include "SEGA\App.h"

#include <stdio.h>

typedef struct{
   long startTime;//ms
   int startX, startY;
   int destX, destY;
}TInterpolationComponent;

#define TComponentT TInterpolationComponent
#include "Entities\ComponentDeclTransient.h"

struct InterpolationManager_t{
   Manager m;
   EntitySystem *system;
   vec(EntityPtr) *removeList;
   long pausedTime;
   bool paused;
};

#pragma region vtable things
static void InterpolationManagerDestroy(InterpolationManager*);
static void InterpolationManagerOnDestroy(InterpolationManager*, Entity*);
static void InterpolationManagerOnUpdate(InterpolationManager*, Entity*);

static ManagerVTable *_createVTable(){
   static ManagerVTable *out = NULL;

   if (!out){
      out = calloc(1, sizeof(ManagerVTable));
      out->destroy = (void(*)(Manager*))&InterpolationManagerDestroy;
      out->onDestroy = (void(*)(Manager*, Entity*))&InterpolationManagerOnDestroy;
      out->onUpdate = (void(*)(Manager*, Entity*))&InterpolationManagerOnUpdate;
   }

   return out;
}

#pragma endregion

InterpolationManager *createInterpolationManager(EntitySystem *system){
   InterpolationManager *out = checkedCalloc(1, sizeof(InterpolationManager));
   out->system = system;
   out->m.vTable = _createVTable();
   out->removeList = vecCreate(EntityPtr)(NULL);
   return out;
}

void InterpolationManagerDestroy(InterpolationManager *self){
   vecDestroy(EntityPtr)(self->removeList);
   checkedFree(self);
}

void InterpolationManagerOnDestroy(InterpolationManager *self, Entity *e){}
void InterpolationManagerOnUpdate(InterpolationManager *self, Entity *e){
   TInterpolationComponent *tic = entityGet(TInterpolationComponent)(e);
   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);

   if (pc){
      if (ic){
         if (tic){
            //already has one
            if (ic->destX != tic->destX || ic->destY != tic->destY){
               //destination has changed, u0date it
               tic->startTime = (long)appGetTime(appGet());
               tic->startX = pc->x;
               tic->startY = pc->y;
               tic->destX = ic->destX;
               tic->destY = ic->destY;
            }
         }
         else{
            //new grid entry
            COMPONENT_ADD(e, TInterpolationComponent, .startTime = (long)appGetTime(appGet()), 
                                                          .startX = pc->x, 
                                                          .startY = pc->y,
                                                          .destX = ic->destX,
                                                          .destY = ic->destY);
         }
      }
      else{
         if (tic){
            //no longer rendered
            entityRemove(TInterpolationComponent)(e);
         }
      }
   }
}

void _updateEntity(InterpolationManager *self, Entity *e, long time){
   TInterpolationComponent *tic = entityGet(TInterpolationComponent)(e);
   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);

   if (ic && pc){
      double m = (time - tic->startTime) / (ic->time * 1000.0);
      if (m > 1.0){
         m = 1.0;
         vecPushBack(EntityPtr)(self->removeList, &e);
      }

      pc->x = (int)((ic->destX - tic->startX) * m) + tic->startX;
      pc->y = (int)((ic->destY - tic->startY) * m) + tic->startY;
   }
}
void interpolationManagerPause(InterpolationManager *self){
   if (!self->paused){
      self->paused = true;
      self->pausedTime = (long)appGetTime(appGet());
   }
}
void interpolationManagerResume(InterpolationManager *self){
   if (self->paused){
      long elapsed = (long)appGetTime(appGet()) - self->pausedTime;
      self->paused = false;

      COMPONENT_QUERY(self->system, TInterpolationComponent, tic, {
         tic->startTime += elapsed;
      });
   }
}

void _removeComponents(InterpolationManager *self){

   vecForEach(EntityPtr, e, self->removeList, {
      entityRemove(InterpolationComponent)(*e);
      entityRemove(TInterpolationComponent)(*e);
   });

   vecClear(EntityPtr)(self->removeList);
}

void interpolationManagerUpdate(InterpolationManager *self){
   if (!self->paused){
      long time = (long)appGetTime(appGet());

      COMPONENT_QUERY(self->system, TInterpolationComponent, ic, {
         Entity *e = componentGetParent(ic, self->system);
         _updateEntity(self, e, time);
      });

      if (!vecIsEmpty(EntityPtr)(self->removeList)){
         _removeComponents(self);
      }
   }
}
