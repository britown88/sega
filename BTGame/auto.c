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
#include "GridHelpers.h"

typedef struct {
   WorldView *view;
   Action *a;
}AutoRoutineData;

static AutoRoutineData *AutoRoutineDataCreate(){
   return checkedCalloc(1, sizeof(AutoRoutineData));
}

static void _AutoRoutineDestroy(AutoRoutineData *self){
   checkedFree(self);
}

static CoroutineStatus _AutoRoutine(AutoRoutineData *data, CoroutineRequest request){
   BTManagers *managers = data->view->managers;
   ActionUserComponent *uc = entityGet(ActionUserComponent)(data->a);
   float range = 0.0;
   Entity *e;

   if (!uc){
      //bail out if we dont hav a user   
      return Finished;
   }

   e = uc->user;
   range = commandManagerGetSlotRange(managers->commandManager, e, 0);
   if (range > 0.0f){
      GridComponent *gc = entityGet(GridComponent)(e);
      TeamComponent *tc = entityGet(TeamComponent)(e);
      if (gc && tc){
         Entity *target = gridFindClosestEntity(
            managers->gridManager,
            gridIndexFromXY(gc->x, gc->y),
            !tc->teamID,
            range);

         if (target){
            entitySetPrimaryTargetEntity(e, target);
         }
      }
   }

   return Finished;
}

static Coroutine _buildAuto(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   AutoRoutineData *newData = AutoRoutineDataCreate();
   newData->view = view;
   newData->a = a;
   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_AutoRoutine, &_AutoRoutineDestroy);
   return out;
}

CombatRoutineGenerator buildAutoAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildAuto, NULL);
   return out;
}
