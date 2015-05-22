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
   long startTime, pausedTime;
   bool paused;
   float range;
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
   ActionAbilityNameComponent *anc = entityGet(ActionAbilityNameComponent)(data->a);
   Entity *e, *target;

   if (!uc || !tec || !anc){
      //shouldnt get here but return done if we dont ahve the right components!
      return Finished;
   }

   e = uc->user;
   target = tec->target;

   if (request == Pause && !data->paused){
      data->paused = true;
      data->pausedTime = gameClockGetTime(data->view->gameClock);
      return NotFinished;
   }

   if (data->paused){
      data->paused = false;
      if (data->startTime > 0){
         data->startTime += gameClockGetTime(data->view->gameClock) - data->pausedTime;
      }
   }

   if (entityIsDead(target)){
      if (entityShouldAutoAttack(e)){
         actionHelperPushFrontAbility(managers->commandManager, e, NULL, stringIntern("auto"));
      }
      return Finished;
   }

   //we're not currently in an attack
   if (gridDistance(e, target) > data->range){

      //not in bow range, we need to push a move command and return
      actionHelperPushFrontAbility(managers->commandManager, e, target, anc->ability);
      actionHelperPushFrontMoveToEntity(managers->commandManager, e, target, data->range);
      return Finished;
   }
   else{
      if (data->startTime == 0){
         data->action = combatManagerCreateAction(managers->combatManager, e, target);
         COMPONENT_ADD(data->action, CActionDamageComponent, .damage = 30.0f);
         COMPONENT_ADD(data->action, CActionRangeComponent, .range = data->range);
         COMPONENT_ADD(data->action, CActionDamageTypeComponent, .type = DamageTypePhysical);

         data->action = combatManagerDeclareAction(managers->combatManager, data->action);

         if (entityGet(CActionCancelledComponent)(data->action)){
            //action got cancelled, bail The F Out
            return Finished;
         }

         //start the timer
         data->startTime = gameClockGetTime(data->view->gameClock);
         return requestIsCancel(request) ? Finished : NotFinished;
      }
      else{
         long elapsed = gameClockGetTime(data->view->gameClock) - data->startTime;
         if (elapsed < 775){
            //clock is still going, but cancellable so
            return requestIsCancel(request) ? Finished : NotFinished;
         }
         else{         
            //timers done lets create our action
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
            COMPONENT_ADD(projectile, ImageComponent, stringIntern("assets/img/dota/venge.ega"));
            COMPONENT_ADD(projectile, LayerComponent, LayerTokens);
            COMPONENT_ADD(projectile, InterpolationComponent, .destX = pc2->x, .destY = pc2->y, .time = 0.25);
            entityUpdate(projectile);

            //attach our modified combataction to the projectile's command
            a = actionCreateCustom(managers->commandManager);
            COMPONENT_ADD(a, ActionTargetEntityComponent, target);
            COMPONENT_ADD(a, ActionRoutineComponent, stringIntern("projectile"));
            COMPONENT_ADD(a, ActionDeliveryComponent, data->action);
            entityPushCommand(projectile, a);

            if (!requestIsCancel(request)){
               actionHelperPushFrontAbility(managers->commandManager, e, target, stringIntern("auto"));
            }

            return Finished;
         }         
      }           
   }

   return Finished;
}

static Coroutine _buildBow(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   BowRoutineData *newData = bowRoutineDataCreate();
   ActionRangeComponent *arc = entityGet(ActionRangeComponent)(a);

   newData->view = view;
   newData->a = a;
   newData->range = arc ? arc->range : 0.0f;


   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_bowRoutine, &_bowRoutineDestroy);

   return out;
}

CombatRoutineGenerator buildBowAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildBow, NULL);
   return out;
}
