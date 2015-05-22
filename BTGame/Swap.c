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

#define SWAP_RANGE 10

typedef struct {
   WorldView *view;
   Action *a;
   CombatAction *action;
   long startTime, pausedTime;
   bool paused;
}SwapRoutineData;

static SwapRoutineData *SwapRoutineDataCreate(){
   return checkedCalloc(1, sizeof(SwapRoutineData));
}

static void _SwapRoutineDestroy(SwapRoutineData *self){
   checkedFree(self);
}

static CoroutineStatus _SwapRoutine(SwapRoutineData *data, CoroutineRequest request){
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

   entitySetPrimaryTargetEntity(e, target);

   //we're not currently in an attack
   if (gridDistance(e, target) > SWAP_RANGE){
      Action *cmd = cc->type == ccSlot ?
         createActionCombatSlot(managers->commandManager, cc->slot, target) :
         createActionCombatRoutine(managers->commandManager, cc->routine, target);

      //not in melee range, we need to push a move command and return   
      entityPushFrontCommand(e, cmd);
      entityPushFrontCommand(e, createActionGridTarget(managers->commandManager, target, SWAP_RANGE, false));
      return Finished;
   }
   else{
      if (data->startTime == 0){
         data->action = combatManagerCreateAction(managers->combatManager, e, target);
         COMPONENT_ADD(data->action, CActionRangeComponent, .range = SWAP_RANGE);

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
            Action *a;

            data->action = combatManagerQueryActionResult(managers->combatManager, data->action);
            if (entityGet(CActionCancelledComponent)(data->action)){
               return Finished;               
            }

            combatManagerExecuteAction(managers->combatManager, data->action);

            //completely wipe all of the targets actions
            //entityPauseCommand(target);
            //entityForceCancelAllCommands(target);
            //entityClearPrimaryTarget(target);

            //attach our modified combataction to the projectile's command
            a = createActionCombatRoutine(managers->commandManager, stringIntern("swap-other"), e);            
            //entityPushFrontCommand(target, a);

            entityPauseCommand(target, a);

            //kill our target focus and push an autoattack for after the swap
            entityClearPrimaryTarget(e);
            //entityPushFrontCommand(e, createActionCombatRoutine(managers->commandManager, stringIntern("auto"), NULL));

            //pushfront our own swap
            entityPushFrontCommand(e, createActionCombatRoutine(managers->commandManager, stringIntern("swap-other"), target));

            return Finished;
         }
      }
   }

   return Finished;

}

static Coroutine _buildSwap(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   SwapRoutineData *newData = SwapRoutineDataCreate();
   newData->view = view;
   newData->a = a;
   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_SwapRoutine, &_SwapRoutineDestroy);

   COMPONENT_ADD(a, ActionRangeComponent, SWAP_RANGE);

   return out;
}

CombatRoutineGenerator buildSwapAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildSwap, NULL);
   return out;
}
