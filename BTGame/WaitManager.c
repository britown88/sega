#include "Managers.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "segautils\Defs.h"

#include "SEGA\App.h"
#include "WorldView.h"
#include "GameClock.h"


typedef struct {
   Microseconds startTime;
}TWaitComponent;

#define TComponentT TWaitComponent
#include "Entities\ComponentDeclTransient.h"

struct WaitManager_t {
   Manager m;
   WorldView *view;
   vec(EntityPtr) *removeList;
};

ImplManagerVTable(WaitManager)

WaitManager *createWaitManager(WorldView *view) {
   WaitManager *out = checkedCalloc(1, sizeof(WaitManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(WaitManager);
   out->removeList = vecCreate(EntityPtr)(NULL);
   return out;
}

void _destroy(WaitManager *self) {
   vecDestroy(EntityPtr)(self->removeList);
   checkedFree(self);
}
void _onDestroy(WaitManager *self, Entity *e) {}
void _onUpdate(WaitManager *self, Entity *e) {
   TWaitComponent *twc = entityGet(TWaitComponent)(e);
   WaitComponent *wc = entityGet(WaitComponent)(e);

   if (wc) {
      if (twc) {
         //already has one, update it with a new starttime
         twc->startTime = gameClockGetTime(self->view->gameClock) - wc->overflow;
      }
      else {
         //add a new transient
         COMPONENT_ADD(e, TWaitComponent, gameClockGetTime(self->view->gameClock) - wc->overflow);
      }

      wc->overflow = 0;
   }
   else if (twc) {
      //wait comp was removed somehow, cleanup the transient
      entityRemove(TWaitComponent)(e);
   }
}

static void _removeComponents(WaitManager *self) {
   vecForEach(EntityPtr, e, self->removeList, {
      entityRemove(TWaitComponent)(*e);
   });

   vecClear(EntityPtr)(self->removeList);
}

static void _updateEntity(WaitManager *self, Entity *e, Microseconds time) {
   TWaitComponent *twc = entityGet(TWaitComponent)(e);
   WaitComponent *wc = entityGet(WaitComponent)(e);

   if (wc) {
      Microseconds delta = time - twc->startTime;
      Microseconds target = t_m2u(wc->time);

      double m = delta / (double)target;
      if (m > 1.0) {
         m = 1.0;
         wc->overflow = delta - target;
         vecPushBack(EntityPtr)(self->removeList, &e);
      }
   }
}

void waitManagerUpdate(WaitManager *self) {
   COMPONENT_QUERY(self->view->entitySystem, TWaitComponent, twc, {
      Entity *e = componentGetParent(twc, self->view->entitySystem);
      _updateEntity(self, e, gameClockGetTime(self->view->gameClock));
   });

   if (!vecIsEmpty(EntityPtr)(self->removeList)) {
      _removeComponents(self);
   }
}