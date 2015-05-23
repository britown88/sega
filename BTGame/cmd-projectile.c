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
   CombatAction *action;
}ProjectileRoutineData;

static ProjectileRoutineData *projectileRoutineDataCreate(){
   return checkedCalloc(1, sizeof(ProjectileRoutineData));
}

static void _projectileRoutineDestroy(ProjectileRoutineData *self){
   checkedFree(self);
}

static CoroutineStatus _projectileRoutine(ProjectileRoutineData *data, CoroutineRequest request){
   BTManagers *managers = data->view->managers;
   ActionUserComponent *uc = entityGet(ActionUserComponent)(data->a);
   ActionTargetEntityComponent *tec = entityGet(ActionTargetEntityComponent)(data->a);
   Entity *e, *target;

   if (!uc || !tec){
      //shouldnt get here but return done if we dont ahve the right components!      
      return Finished;
   }

   e = uc->user;
   target = tec->target;

   if (request == Pause){
      return NotFinished;
   }

   if (requestIsCancel(request)){
      COMPONENT_ADD(e, DestructionComponent, 0);
      return Finished;
   }   

   if (!entityGet(InterpolationComponent)(e)){
      ActionDeliveryComponent *delivery = entityGet(ActionDeliveryComponent)(data->a);

      if (entityIsDead(target)){
         COMPONENT_ADD(e, DestructionComponent, 0);
         return Finished;
      }

      if (delivery){
         data->action = delivery->package;
         data->action = combatManagerQueryActionResult(managers->combatManager, data->action);
         if (!entityGet(CActionCancelledComponent)(data->action)){
            Status *s = statusCreate(managers->statusManager);
            COMPONENT_ADD(s, StatusNameComponent, stringIntern("stun"));
            COMPONENT_ADD(s, StatusDurationComponent, 250);
            COMPONENT_ADD(s, StatusInflictsStunComponent, 0);

            entityAddStatus(managers->statusManager, target, s);

            combatManagerExecuteAction(managers->combatManager, data->action);


         }
      }

      COMPONENT_ADD(e, DestructionComponent, 0);
      return Finished;
   }

   return NotFinished;
}

static Coroutine _buildProjectile(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   ProjectileRoutineData *newData = projectileRoutineDataCreate();
   newData->view = view;
   newData->a = a;
   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_projectileRoutine, &_projectileRoutineDestroy);
   return out;
}

CombatRoutineGenerator buildProjectileAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildProjectile, NULL);
   return out;
}
