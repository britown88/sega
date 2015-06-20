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
}SwapRoutineData;

static SwapRoutineData *SwapRoutineDataCreate(){
   return checkedCalloc(1, sizeof(SwapRoutineData));
}

static void _SwapRoutineDestroy(SwapRoutineData *self){
   checkedFree(self);
}

static void _pushSwapOtherAction(CommandManager *manager, Entity *user, Entity *target){
   //push a custom swap-other command
   Action *a = actionCreateCustom(manager);
   COMPONENT_ADD(a, ActionTargetEntityComponent, target);
   COMPONENT_ADD(a, ActionRoutineComponent, stringIntern("swap-other"));
   entityPushFrontCommand(user, a);
}

static CoroutineStatus _SwapRoutine(SwapRoutineData *data, CoroutineRequest request){
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

   if (entityIsDead(target) || requestIsCancel(request)){
      return Finished;
   }

   //we're not currently in an attack
   if (gridDistance(e, target) > data->range){
      actionHelperPushFrontAbility(managers->commandManager, e, target, anc->ability);
      actionHelperPushFrontMoveToEntity(managers->commandManager, e, target, data->range);
      return Finished;
   }
   else{
      if (data->startTime == 0){
         data->action = combatManagerCreateAction(managers->combatManager, e, target);
         COMPONENT_ADD(data->action, CActionRangeComponent, .range = data->range);

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
         if (elapsed < 250){
            //clock is still going, but cancellable so
            return requestIsCancel(request) ? Finished : NotFinished;
         }
         else{
            //timers done lets create our action
            data->action = combatManagerQueryActionResult(managers->combatManager, data->action);
            if (entityGet(CActionCancelledComponent)(data->action)){
               return Finished;               
            }

            combatManagerExecuteAction(managers->combatManager, data->action);

            entityForceCancelAllCommands(target);
            _pushSwapOtherAction(managers->commandManager, target, e);     

            //pushfront our own commands, we want to swap and then do an auto attack
            if (entityShouldAutoAttack(e)){
               if (entitiesAreEnemies(e, target)){
                  actionHelperPushFrontAbility(managers->commandManager, e, target, stringIntern("auto"));
               }
               else{
                  actionHelperPushFrontAbility(managers->commandManager, e, NULL, stringIntern("auto"));
               }
               
            }

            //push a custom swap-other command
            _pushSwapOtherAction(managers->commandManager, e, target);

            return Finished;
         }
      }
   }

   return Finished;

}

static Coroutine _buildSwap(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   SwapRoutineData *newData = SwapRoutineDataCreate();
   ActionRangeComponent *arc = entityGet(ActionRangeComponent)(a);

   newData->view = view;
   newData->a = a;
   newData->range = arc ? arc->range : 0.0f;

   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_SwapRoutine, &_SwapRoutineDestroy);


   return out;
}

CombatRoutineGenerator buildSwapAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildSwap, NULL);
   return out;
}
