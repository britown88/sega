#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"
#include "Combat.h"
#include "Actions.h"
#include "GridManager.h"

typedef enum{
   ptPosition,
   ptEntity
}PrimaryTargetType;

typedef struct{
   PrimaryTargetType type;
   union {
      size_t pos;
      Entity *e;
   };
}TPrimaryTargetComponent;

#define TComponentT TPrimaryTargetComponent
#include "Entities\ComponentDeclTransient.h"

static void _removeListRemove(EntityPtr *self){
   entityRemove(TPrimaryTargetComponent)(*self);
}

struct PrimaryTargetManager_t{
   Manager m;
   WorldView *view;

   vec(EntityPtr) *removeList;

};

ImplManagerVTable(PrimaryTargetManager)

PrimaryTargetManager *createPrimaryTargetManager(WorldView *view){
   PrimaryTargetManager *out = checkedCalloc(1, sizeof(PrimaryTargetManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(PrimaryTargetManager);
   out->removeList = vecCreate(EntityPtr)(&_removeListRemove);
   return out;
}

void _destroy(PrimaryTargetManager *self){
   vecDestroy(EntityPtr)(self->removeList);
   checkedFree(self);
}
void _onDestroy(PrimaryTargetManager *self, Entity *e){}
void _onUpdate(PrimaryTargetManager *self, Entity *e){}

static void _updateEntity(PrimaryTargetManager *self, Entity *e){
   //if we arent doing antyhign we should push commands toward our target
   TPrimaryTargetComponent *tptc = entityGet(TPrimaryTargetComponent)(e);
   TeamComponent *tc = entityGet(TeamComponent)(e);
   GridComponent *gc = entityGet(GridComponent)(e);
   CommandComponent *cc = entityGet(CommandComponent)(e);
   BTManagers *managers = self->view->managers;

   if (!tc || !tptc || !gc){
      //wrong comps!
      return;
   }

   if (cc && !vecIsEmpty(ActionPtr)(cc->actions)){
      //currently doing shit, ignore our target
      return;
   }

   if (tptc->type == ptPosition){
      int x, y;
      gridXYFromIndex(tptc->pos, &x, &y);

      if (tptc->pos == gridIndexFromXY(gc->x, gc->y)){
         //reached our dest, kill this component
         vecPushBack(EntityPtr)(self->removeList, &e);
      }
      else{
         entityPushCommand(e, createActionGridPosition(managers->commandManager, x, y));
      }
   }else if (tptc->type == ptEntity && tptc->e){
      //targeting an entity, find its alignment
      Entity *target = tptc->e;
      TeamComponent *tcOther = entityGet(TeamComponent)(target);

      if (entityIsDead(target)){
         //our target is dead, push an auto-attack and flag the targetcomponent for removal
         entityPushCommand(e, createActionCombatRoutine(managers->commandManager, stringIntern("auto"), NULL));
         vecPushBack(EntityPtr)(self->removeList, &e);         
         return;
      }

      if (tcOther){
         if (tc->teamID == tcOther->teamID){
            //same team
            entityPushCommand(e, createActionGridTarget(managers->commandManager, target, 0));
         }
         else{
            //opposing teams
            entityPushCommand(e, createActionCombatSlot(managers->commandManager, 0, target));
         }
      }
   }
}

void primaryTargetManagerUpdate(PrimaryTargetManager *self){
   COMPONENT_QUERY(self->view->entitySystem, TPrimaryTargetComponent, tptc, {
      Entity *e = componentGetParent(tptc, self->view->entitySystem);
      _updateEntity(self, e);
   });

   vecClear(EntityPtr)(self->removeList);
}

void entitySetPrimaryTargetPosition(Entity *e, size_t gridIndex){
   TPrimaryTargetComponent *tptc = entityGet(TPrimaryTargetComponent)(e);

   if (!tptc){
      COMPONENT_ADD(e, TPrimaryTargetComponent, ptPosition, .pos = gridIndex );
   }
   else{
      *tptc = (TPrimaryTargetComponent){ ptPosition, .pos = gridIndex };
   }
}

void entitySetPrimaryTargetEntity(Entity *e, Entity *other){
   TPrimaryTargetComponent *tptc = entityGet(TPrimaryTargetComponent)(e);

   if (!tptc){
      COMPONENT_ADD(e, TPrimaryTargetComponent, ptEntity, .e = other);
   }
   else{
      *tptc = (TPrimaryTargetComponent){ ptEntity, .e = other };
   }
}