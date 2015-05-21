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
#include "GameClock.h"

typedef struct {
   WorldView *view;
   Action *a;
   CombatAction *action;
   long startTime;
}BowRoutineData;

static BowRoutineData *bowRoutineDataCreate(){
   return checkedCalloc(1, sizeof(BowRoutineData));
}

static void _bowRoutineDestroy(BowRoutineData *self){
   checkedFree(self);
}

static CoroutineStatus _bowRoutine(BowRoutineData *data, CoroutineRequest request){
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


   if (entityIsDead(target) || requestIsCancel(request)){
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
      if (data->startTime > 0){
         if (gameClockGetTime(data->view->gameClock) - data->startTime > 1000){
            //timers done lets create our action
            data->action = combatManagerCreateAction(managers->combatManager, e, target);
            COMPONENT_ADD(data->action, CActionDamageComponent, .damage = 30.0f);
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
               Entity *projectile;
               Action *a;

               if (!pc || !pc2){
                  return Finished;
               }

               projectile = entityCreate(data->view->entitySystem);

               //we're in range, our declaration's been accepted, lets go!
               COMPONENT_ADD(projectile, PositionComponent, .x = pc->x, .y = pc->y);
               COMPONENT_ADD(projectile, ImageComponent, stringIntern("assets/img/cursor.ega"));
               COMPONENT_ADD(projectile, LayerComponent, LayerUI);
               COMPONENT_ADD(projectile, CombatSlotsComponent, .slots = { stringIntern("projectile"), NULL });
               COMPONENT_ADD(projectile, InterpolationComponent, .destX = pc2->x, .destY = pc2->y, .time = 0.25);
               entityUpdate(projectile);

               //attach our modified combataction to the projectile's command
               a = createActionCombat(managers->commandManager, 0, target);
               COMPONENT_ADD(a, ActionDeliveryComponent, data->action);
               entityPushCommand(projectile, a);

               if (!requestIsCancel(request) && !entityIsDead(target)){
                  //we're not cancelling so keep hittin the dude
                  entityPushFrontCommand(e, createActionCombat(managers->commandManager, 0, target));
               }

               return Finished;
            }
         }
         else {
            //clock is still going, but cancellable so
            return requestIsCancel(request) ? Finished : NotFinished;
         }
      }
      else{
         //start the timer
         data->startTime = gameClockGetTime(data->view->gameClock);
         return requestIsCancel(request) ? Finished : NotFinished;
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
