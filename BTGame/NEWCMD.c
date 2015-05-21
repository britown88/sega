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
}NEWCMDRoutineData;

static NEWCMDRoutineData *NEWCMDRoutineDataCreate(){
   return checkedCalloc(1, sizeof(NEWCMDRoutineData));
}

static void _NEWCMDRoutineDestroy(NEWCMDRoutineData *self){
   checkedFree(self);
}

static CoroutineStatus _NEWCMDRoutine(NEWCMDRoutineData *data, CoroutineRequest request){
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

   return Finished;
}

static Coroutine _buildNEWCMD(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   NEWCMDRoutineData *newData = NEWCMDRoutineDataCreate();
   newData->view = view;
   newData->a = a;
   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_NEWCMDRoutine, &_NEWCMDRoutineDestroy);
   return out;
}

CombatRoutineGenerator buildNEWCMDAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildNEWCMD, NULL);
   return out;
}