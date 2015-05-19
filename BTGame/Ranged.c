#include <math.h>

#include "segashared\CheckedMemory.h"

#include "Actions.h"
#include "Commands.h"
#include "Managers.h"
#include "CombatRoutines.h"
#include "GridManager.h"
#include "CoreComponents.h"
#include "LogManager.h"
#include "Combat.h"

typedef struct {
   WorldView *view;
   Action *a;
   CombatAction *action;
}BowRoutineData;

static BowRoutineData *bowRoutineDataCreate(){
   return checkedCalloc(1, sizeof(BowRoutineData));
}

static void _bowRoutineDestroy(BowRoutineData *self){
   checkedFree(self);
}

static CoroutineStatus _bowRoutine(BowRoutineData *data, bool cancel){
   BTManagers *managers = data->view->managers;
   ActionUserComponent *uc = entityGet(ActionUserComponent)(data->a);
   ActionTargetEntityComponent *tec = entityGet(ActionTargetEntityComponent)(data->a);
   ActionCombatComponent *cc = entityGet(ActionCombatComponent)(data->a);
   Entity *e, *target;

   if (!uc || !tec || !cc){
      //shouldnt get here but return done if we dont ahve the right components!
      return Finished;
   }

   e = uc->user;
   target = tec->target;


   if (entityIsDead(target)){
      return Finished;
   }

   //we're not currently in an attack
   if (gridDistance(e, target) > 5){
      //not in melee range, we need to push a move command and return   
      logManagerPushMessage(managers->logManager, "Moving to attack.");
      entityPushFrontCommand(e, createActionCombat(managers->commandManager, cc->slot, target));
      entityPushFrontCommand(e, createActionGridTarget(managers->commandManager, target, 5.0f));
      return Finished;
   }
   else{
      data->action = combatManagerCreateAction(managers->combatManager, e, target);

      COMPONENT_ADD(data->action, CActionDamageComponent, .damage = 20.0f);
      COMPONENT_ADD(data->action, CActionRangeComponent, .range = 5.0f);
      COMPONENT_ADD(data->action, CActionDamageTypeComponent, .type = DamageTypePhysical);

      data->action = combatManagerDeclareAction(managers->combatManager, data->action);

      if (entityGet(CActionCancelledComponent)(data->action)){
         //action got cancelled, bail The F Out
         return Finished;
      }
      else{
         PositionComponent *pc = entityGet(PositionComponent)(e);
         PositionComponent *pc2 = entityGet(PositionComponent)(target);
         Entity *projectile = entityCreate(data->view->entitySystem);

         if (!pc || !pc2){
            return Finished;
         }

         //we're in range, our declaration's been accepted, lets go!
         COMPONENT_ADD(projectile, PositionComponent, .x = pc->x, .y = pc->y);
         COMPONENT_ADD(projectile, ImageComponent, stringIntern("assets/img/cursor.ega"));
         COMPONENT_ADD(projectile, LayerComponent, LayerUI);
         COMPONENT_ADD(projectile, CombatSlotsComponent, .slots = { stringIntern("projectile"), NULL });
         COMPONENT_ADD(projectile, InterpolationComponent, .destX = pc2->x, .destY = pc2->y, .time = 0.25);
         entityUpdate(projectile);

         entityPushCommand(projectile, createActionCombat(managers->commandManager, 0, target));

         


         return Finished;
      }
   }

   return Finished;
}

static Coroutine _buildBow(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   BowRoutineData *newData = bowRoutineDataCreate();
   newData->view = view;
   newData->a = a;
   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_bowRoutine, &_bowRoutineDestroy);
   return out;
}

CombatRoutineGenerator buildBowAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildBow, NULL);
   return out;
}
