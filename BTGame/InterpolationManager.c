#include "Managers.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "segautils\Defs.h"

#include "SEGA\App.h"
#include "WorldView.h"
#include "GameClock.h"

#include <stdio.h>

typedef struct {
   Microseconds startTime;
   int startX, startY;
   int destX, destY;

}TInterpolationComponent;

#define TComponentT TInterpolationComponent
#include "Entities\ComponentDeclTransient.h"

struct InterpolationManager_t {
   Manager m;
   WorldView *view;
   vec(EntityPtr) *removeList;
};

ImplManagerVTable(InterpolationManager)



InterpolationManager *createInterpolationManager(WorldView *view) {
   InterpolationManager *out = checkedCalloc(1, sizeof(InterpolationManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(InterpolationManager);
   out->removeList = vecCreate(EntityPtr)(NULL);
   return out;
}

void _destroy(InterpolationManager *self) {
   vecDestroy(EntityPtr)(self->removeList);
   checkedFree(self);
}

void _onDestroy(InterpolationManager *self, Entity *e) {}
void _onUpdate(InterpolationManager *self, Entity *e) {
   TInterpolationComponent *tic = entityGet(TInterpolationComponent)(e);
   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   
   if (pc) {
      if (ic) {
         if (tic) {
            //already has one
            if (ic->destX != tic->destX || ic->destY != tic->destY) {
               //destination has changed, u0date it
               tic->startTime = gameClockGetTime(self->view->gameClock) - ic->overflow;
               tic->startX = pc->x;
               tic->startY = pc->y;
               tic->destX = ic->destX;
               tic->destY = ic->destY; 
            }
         }
         else {
            //new entry, add a transient
            COMPONENT_ADD(e, TInterpolationComponent, 
               .startTime = gameClockGetTime(self->view->gameClock) - ic->overflow,
               .startX = pc->x,
               .startY = pc->y,
               .destX = ic->destX,
               .destY = ic->destY);

            //_updateEntity(self, e, gameClockGetTime(self->view->gameClock));
         }

         ic->overflow = 0;
      }
      else {
         if (tic) {
            //interpolation comp was removed somehow, cleanup the transient
            entityRemove(TInterpolationComponent)(e);
         }
      }
   }
}

static void _removeComponents(InterpolationManager *self) {
   vecForEach(EntityPtr, e, self->removeList, {      
      entityRemove(TInterpolationComponent)(*e);
   });

   vecClear(EntityPtr)(self->removeList);
}

static void _updateEntity(InterpolationManager *self, Entity *e, Microseconds time) {
   TInterpolationComponent *tic = entityGet(TInterpolationComponent)(e);
   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);

   if (ic && pc) {
      Microseconds delta = time - tic->startTime;
      Microseconds target = t_m2u(ic->time);

      double m = delta / (double)target;
      if (m > 1.0) {
         m = 1.0;
         ic->overflow = delta - target;
         vecPushBack(EntityPtr)(self->removeList, &e);
      }

      pc->x = (int)((ic->destX - tic->startX) * m) + tic->startX;
      pc->y = (int)((ic->destY - tic->startY) * m) + tic->startY;
   }
}

void interpolationManagerUpdate(InterpolationManager *self) {
   COMPONENT_QUERY(self->view->entitySystem, TInterpolationComponent, ic, {
      Entity *e = componentGetParent(ic, self->view->entitySystem);
      _updateEntity(self, e, gameClockGetTime(self->view->gameClock));
   });

   if (!vecIsEmpty(EntityPtr)(self->removeList)) {
      _removeComponents(self);
   }
}
