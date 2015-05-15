#include <math.h>

#include "segashared\CheckedMemory.h"

#include "Actions.h"
#include "Commands.h"
#include "Managers.h"
#include "CombatRoutines.h"
#include "GridManager.h"
#include "CoreComponents.h"
#include "LogManager.h"

typedef struct {
   WorldView *view;
   Action *a;
}MeleeRoutineData;

static MeleeRoutineData *meleeRoutineDataCreate(){
   return checkedCalloc(1, sizeof(MeleeRoutineData));
}

static void _meleeRoutineDestroy(MeleeRoutineData *self){
   checkedFree(self);
}

static int _distance(Entity *user, Entity *target){
   GridComponent *gc0 = entityGet(GridComponent)(user);
   GridComponent *gc1 = entityGet(GridComponent)(target);

   return abs(gc0->x - gc1->x) + abs(gc0->y - gc1->y);
}

static CoroutineStatus _meleeRoutine(MeleeRoutineData *data, bool cancel){
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

   if (_distance(e, target) > 1){
      //not in melee range, we need to push a move command and return   
      logManagerPushMessage(managers->logManager, "Moving to attack.");
      entityPushFrontCommand(e, createActionCombat(managers->commandManager, cc->slot, target));
      entityPushFrontCommand(e, createActionGridTarget(managers->commandManager, target));
      return Finished;
   }
   else{
      logManagerPushMessage(managers->logManager, "Attacked!");
      return Finished;
   }

   return Finished;
}

static Coroutine _buildMelee(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   MeleeRoutineData *newData = meleeRoutineDataCreate();
   newData->view = view;
   newData->a = a;
   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_meleeRoutine, &_meleeRoutineDestroy);
   return out;
}

CombatRoutineGenerator buildMeleeAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildMelee, NULL);
   return out;
}

Action *createActionCombat(CommandManager *self, size_t slot, Entity *e){
   Action *a = commandManagerCreateAction(self);
   COMPONENT_ADD(a, ActionTargetEntityComponent, e);
   COMPONENT_ADD(a, ActionCombatComponent, .slot = slot);
   entityUpdate(a);

   return a;
}