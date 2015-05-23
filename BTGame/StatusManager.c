#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"
#include "StatusManager.h"
#include "GameClock.h"

#define ComponentT StatusNameComponent
#include "Entities\ComponentImpl.h"

#define ComponentT StatusDurationComponent
#include "Entities\ComponentImpl.h"

#define ComponentT StatusInflictsStunComponent
#include "Entities\ComponentImpl.h"

#define ComponentT StatusParentComponent
#include "Entities\ComponentImpl.h"

#define ComponentT StatusChildActionComponent
#include "Entities\ComponentImpl.h"

typedef struct{
   vec(EntityPtr) *statuses;
}TStatusComponent;

static void _TStatusDestroy(TStatusComponent *self){
   vecDestroy(EntityPtr)(self->statuses);
}

static void _addStatusComponent(Entity *e);

#define COMP_DESTROY_FUNC _TStatusDestroy
#define TComponentT TStatusComponent
#include "Entities\ComponentDeclTransient.h"

typedef Status *StatusPtr;
#define VectorT StatusPtr
#include "segautils\Vector_Create.h"

void _addStatusComponent(Entity *e) {
   COMPONENT_ADD(e, TStatusComponent, vecCreate(EntityPtr)(&entityVectorDestroy));
}

static void _postDestroy(StatusPtr *self){
   Status *s = *self;
   StringView name = entityGet(StatusNameComponent)(s)->name;
   Entity *parent = entityGet(StatusParentComponent)(s)->parent;

   entityRemoveStatus(parent, name);
}

struct StatusManager_t{
   Manager m;
   WorldView *view;

   EntitySystem *statuses;
   vec(StatusPtr) *postDestroy;
};

ImplManagerVTable(StatusManager)

StatusManager *createStatusManager(WorldView *view){
   StatusManager *out = checkedCalloc(1, sizeof(StatusManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(StatusManager);

   out->statuses = entitySystemCreate();
   out->postDestroy = vecCreate(StatusPtr)(&_postDestroy);

   return out;
}

void _destroy(StatusManager *self){
   entitySystemDestroy(self->statuses);
   vecDestroy(StatusPtr)(self->postDestroy);

   checkedFree(self);
}
void _onDestroy(StatusManager *self, Entity *e){}
void _onUpdate(StatusManager *self, Entity *e){}

void statusManagerUpdate(StatusManager *self){

   COMPONENT_QUERY(self->statuses, StatusDurationComponent, dc, {
      Status *s = componentGetParent(dc, self->statuses);
      if (gameClockGetTime(self->view->gameClock) - dc->startTime > dc->duration){

         vecPushBack(StatusPtr)(self->postDestroy, &s);
      }
   });

   vecClear(StatusPtr)(self->postDestroy);
}

Status *statusCreate(StatusManager *self){
   return entityCreate(self->statuses);
}

Status *entityGetStatus(Entity *e, StringView name){
   TStatusComponent *tsc = entityGet(TStatusComponent)(e);

   if (!tsc){
      return NULL;
   }

   vecForEach(EntityPtr, s, tsc->statuses, {
      IF_COMPONENT(*s, StatusNameComponent, snc, {
         if (snc->name == name){
            return *s;
         }
      });
   });

   return NULL;
}

void entityAddStatus(StatusManager *self, Entity *e, Status *s){
   TStatusComponent *tsc = entityGet(TStatusComponent)(e);
   Status *other;

   if (!tsc){
      _addStatusComponent(e);
      tsc = entityGet(TStatusComponent)(e);
   }

   other = entityGetStatus(e, entityGet(StatusNameComponent)(s)->name);

   /*
   depending on if status is stackable or refreshable or w/e, change other
   */

   COMPONENT_ADD(s, StatusParentComponent, e);
   IF_COMPONENT(s, StatusDurationComponent, sdc, {
      sdc->startTime = gameClockGetTime(self->view->gameClock);

      if (other){
         IF_COMPONENT(other, StatusDurationComponent, sdcother, {
            sdcother->duration = sdc->duration;
            sdcother->startTime = sdc->startTime;
         });
      }
   });

   IF_COMPONENT(s, StatusInflictsStunComponent, sisc, {
      if (!other){
         Action *stun = actionFromAbilityName(self->view->managers->commandManager, e, stringIntern("stun"));

         COMPONENT_ADD(s, StatusChildActionComponent, stun);

         entityForceCancelAllCommands(e);
         entityPushCommand(e, stun);
      }
   });

   if (other){
      entityDestroy(s);
   }
   else{
      vecPushBack(EntityPtr)(tsc->statuses, &s);
   }
}

static void _removeStatusPreprocess(Entity *e, Status *s){
   IF_COMPONENT(s, StatusInflictsStunComponent, sisc, {
      IF_COMPONENT(s, StatusChildActionComponent, cac, {
         COMPONENT_ADD(cac->child, ActionInvalidComponent, 0);
      });
   });
}

void entityRemoveStatus(Entity *e, StringView name){
   TStatusComponent *tsc = entityGet(TStatusComponent)(e);
   size_t i = 0;
   if (!tsc){
      return;
   }

   vecForEach(EntityPtr, s, tsc->statuses, {
      IF_COMPONENT(*s, StatusNameComponent, snc, {
         if (snc->name == name){
            _removeStatusPreprocess(e, *s);
            vecRemoveAt(EntityPtr)(tsc->statuses, i);
            return;
         }
      });
      ++i;
   });
}