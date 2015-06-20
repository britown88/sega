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

typedef enum{
   NotAttacked = 0,
   WindUp,
   Swing,
   Recoil
}MoveStage;

typedef struct {
   WorldView *view;
   Action *a;
   MoveStage stage;
   Int2 dir, startPos;
   CombatAction *action;
   bool paused;
}MeleeRoutineData;

static MeleeRoutineData *meleeRoutineDataCreate(){
   return checkedCalloc(1, sizeof(MeleeRoutineData));
}

static void _meleeRoutineDestroy(MeleeRoutineData *self){
   checkedFree(self);
}


static CoroutineStatus _meleeRoutine(MeleeRoutineData *data, CoroutineRequest request){
   ShiftManagers *managers = data->view->managers;
   ActionUserComponent *uc = entityGet(ActionUserComponent)(data->a);
   ActionTargetEntityComponent *tec = entityGet(ActionTargetEntityComponent)(data->a);
   ActionAbilityNameComponent *anc = entityGet(ActionAbilityNameComponent)(data->a);
   Entity *e, *target;
   static int windup = 16;

   if (!uc || !tec || !anc){
      //shouldnt get here but return done if we dont ahve the right components!
      return Finished;
   }

   e = uc->user;
   target = tec->target;

   if (request == ForceCancel){
      if (entityGet(InterpolationComponent)(e)){
         entityRemove(InterpolationComponent)(e);
         entityUpdate(e);
      }
      COMPONENT_LOCK(PositionComponent, pc, e, {
         pc->x = data->startPos.x;
         pc->y = data->startPos.y;
      });

      return Finished;
   }

   if (request == Pause && !data->paused){
      //pause
      data->paused = true;
      if (data->stage != NotAttacked && entityGet(InterpolationComponent)(e)){
         entityRemove(InterpolationComponent)(e);

         COMPONENT_LOCK(PositionComponent, pc, e, {
            pc->x = data->startPos.x;
            pc->y = data->startPos.y;
         });
         entityUpdate(e);

         data->stage = NotAttacked;
      }

      return NotFinished;
   }

   if (data->paused){
      //resume
      data->paused = false;
   }
   
   if (data->stage == NotAttacked){

      if (entityIsDead(target)){
         if (entityShouldAutoAttack(e)){
            Action *cmd = actionFromAbilityName(managers->commandManager, e, stringIntern("auto"));
            cmd = actionTargetEntity(cmd, NULL);
            entityPushFrontCommand(e, cmd);
         }
         return Finished;
      }

      //we're not currently in an attack
      if (gridDistance(e, target) > 1){
         //not in melee range, we need to push a move command and return   
         actionHelperPushFrontAbility(managers->commandManager, e, target, anc->ability);
         actionHelperPushFrontMoveToEntity(managers->commandManager, e, target, 1.0f);

         return Finished;
      }
      else{
         data->action = combatManagerCreateAction(managers->combatManager, e, target);

         COMPONENT_ADD(data->action, CActionDamageComponent, .damage = 45.0f);
         COMPONENT_ADD(data->action, CActionRangeComponent, .range = 1.0f);
         COMPONENT_ADD(data->action, CActionDamageTypeComponent, .type = DamageTypePhysical);

         data->action = combatManagerDeclareAction(managers->combatManager, data->action);

         if (entityGet(CActionCancelledComponent)(data->action)){
            //action got cancelled, bail The F Out
            return Finished;
         }
         else{
            GridComponent *gc0 = entityGet(GridComponent)(e);
            GridComponent *gc1 = entityGet(GridComponent)(target);
            PositionComponent *pc = entityGet(PositionComponent)(e);

            if (!gc0 || !gc1 || !pc){
               return Finished;
            }

            //we're in range, our declaration's been accepted, lets go!
            
            data->stage = WindUp;

            data->startPos = (Int2){ pc->x, pc->y };

            data->dir.x = gc1->x - gc0->x;
            data->dir.y = gc1->y - gc0->y;

            COMPONENT_ADD(e, InterpolationComponent,
               pc->x - (data->dir.x * windup),
               pc->y - (data->dir.y * windup), 0.25);

            entityUpdate(e);

            return NotFinished;
         }
      }
   }
   else if (data->stage == WindUp){
      PositionComponent *pc = entityGet(PositionComponent)(e);

      if (!entityGet(InterpolationComponent)(e)){
         data->stage = Swing;

         COMPONENT_ADD(e, InterpolationComponent,
            pc->x + (data->dir.x * windup * 2),
            pc->y + (data->dir.y * windup * 2), 0.25);
         entityUpdate(e);
      }

      return NotFinished;
   }
   else if (data->stage == Swing){
      PositionComponent *pc = entityGet(PositionComponent)(e);

      if (!entityGet(InterpolationComponent)(e)){
         data->stage = Recoil;

         COMPONENT_ADD(e, InterpolationComponent,
            pc->x - (data->dir.x * windup),
            pc->y - (data->dir.y * windup), 0.25);
         entityUpdate(e);

         //here we are at the end, its time to see if our damage actually gets applied
         data->action = combatManagerQueryActionResult(managers->combatManager, data->action);
         if (!entityGet(CActionCancelledComponent)(data->action)){
            combatManagerExecuteAction(managers->combatManager, data->action);
         }
      }

      return NotFinished;
   }
   else {
      if (!entityGet(InterpolationComponent)(e)){

         if (!requestIsCancel(request)){
            actionHelperPushFrontAbility(managers->commandManager, e, target, stringIntern("auto"));
         }

         //we're done!
         return Finished;
         
      }
      else{
         return NotFinished;
      }
   }

   return Finished;
}

static Coroutine _buildMelee(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   MeleeRoutineData *newData = meleeRoutineDataCreate();
   newData->view = view;
   newData->a = a;
   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_meleeRoutine, &_meleeRoutineDestroy);

   COMPONENT_ADD(a, ActionRangeComponent, 1);

   return out;
}

CombatRoutineGenerator buildMeleeAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildMelee, NULL);
   return out;
}