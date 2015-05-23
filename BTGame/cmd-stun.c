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
#include "StatusManager.h"

typedef struct {
   WorldView *view;
   Action *a;
}StunRoutineData;

static StunRoutineData *StunRoutineDataCreate(){
   return checkedCalloc(1, sizeof(StunRoutineData));
}

static void _StunRoutineDestroy(StunRoutineData *self){
   checkedFree(self);
}

static CoroutineStatus _StunRoutine(StunRoutineData *data, CoroutineRequest request){
   BTManagers *managers = data->view->managers;
   ActionUserComponent *uc = entityGet(ActionUserComponent)(data->a);
   Entity *e;

   if (!uc){
      //shouldnt get here but return done if we dont ahve the right components!      
      return Finished;
   }

   e = uc->user;

   if (entityGet(ActionInvalidComponent)(data->a)){
      return Finished;
   }

   if (request == ForceCancel){
      //close gracefully
      //fine forcecancel me but ill RE-ADD MYSELF, BITCH
      StringView stunName = stringIntern("stun");
      Action *newStun = actionFromAbilityName(data->view->managers->commandManager, e, stunName);

      if (newStun && !entityGet(ActionInvalidComponent)(newStun)){
         //we need to update our parent status with the new command so it can still kill us
         Status *s = entityGetStatus(e, stunName);
         if (s){
            IF_COMPONENT(s, StatusChildActionComponent, cac, {
               cac->child = newStun;
               entityPushNextCommand(e, newStun);
            });
         }
      }
     
      return Finished;
   }

   if (request == Pause){
      //pause

      return NotFinished;
   }


   return NotFinished;
}

static Coroutine _buildStun(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   StunRoutineData *newData = StunRoutineDataCreate();
   newData->view = view;
   newData->a = a;
   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_StunRoutine, &_StunRoutineDestroy);

   //set the range of this action into the action that generated it for ease of looking
   COMPONENT_ADD(a, ActionRangeComponent, 0);

   return out;
}

CombatRoutineGenerator buildStunAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildStun, NULL);
   return out;
}
