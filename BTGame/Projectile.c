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
}ProjectileRoutineData;

static ProjectileRoutineData *projectileRoutineDataCreate(){
   return checkedCalloc(1, sizeof(ProjectileRoutineData));
}

static void _projectileRoutineDestroy(ProjectileRoutineData *self){
   checkedFree(self);
}

static CoroutineStatus _projectileRoutine(ProjectileRoutineData *data, bool cancel){
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
