#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "GridManager.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "CoreComponents.h"
#include <math.h>
#include "segautils\Coroutine.h"
#include "Actions.h"
#include "Commands.h"
#include "WorldView.h"
#include "CombatRoutines.h"
#include "Abilities.h"
#include "Combat.h"

#define ComponentT ActionUserComponent
#include "Entities\ComponentImpl.h"
#define ComponentT ActionTargetEntityComponent
#include "Entities\ComponentImpl.h"
#define ComponentT ActionTargetPositionComponent
#include "Entities\ComponentImpl.h"
#define ComponentT ActionAbilityNameComponent
#include "Entities\ComponentImpl.h"
#define ComponentT ActionRangeComponent
#include "Entities\ComponentImpl.h"
#define ComponentT ActionRoutineComponent
#include "Entities\ComponentImpl.h"
#define ComponentT ActionDeliveryComponent
#include "Entities\ComponentImpl.h"
#define ComponentT ActionInvalidComponent
#include "Entities\ComponentImpl.h"
#define ComponentT ActionGoverningStatusComponent
#include "Entities\ComponentImpl.h"
#define ComponentT ActionPreDelayComponent
#include "Entities\ComponentImpl.h"
#define ComponentT ActionSubActionComponent
#include "Entities\ComponentImpl.h"

#define VectorTPart ActionPtr
#include "segautils\Vector_Impl.h"

typedef struct{
   Coroutine command;
   vec(Coroutine) *pausedCommands;
   bool commandReady;
   size_t runningIndex;//0 by  default
}TCommandComponent;

static void TCommandComponentDestroy(TCommandComponent *self){
   if (self->commandReady){
      closureDestroy(Coroutine)(&self->command);      
   }

   vecForEach(Coroutine, c, self->pausedCommands, {
      closureDestroy(Coroutine)(c);
   });
   vecDestroy(Coroutine)(self->pausedCommands);
}

#define COMP_DESTROY_FUNC TCommandComponentDestroy
#define TComponentT TCommandComponent
#include "Entities\ComponentDeclTransient.h"

typedef struct {
   Entity *e;
   bool add;
}PostRunTransient;

#define VectorT PostRunTransient
#include "segautils\Vector_Create.h"

static void _addNewTCommandComponent(Entity *e){
   TCommandComponent tcc = { 0 };
   tcc.pausedCommands = vecCreate(Coroutine)(NULL);
   entityAdd(TCommandComponent)(e, &tcc);
}

static void _postRunDestroy(PostRunTransient *self){

   if (self->add){
      _addNewTCommandComponent(self->e);
   }
   else{
      entityRemove(TCommandComponent)(self->e);
   }
}

struct CommandManager_t{
   Manager m;
   WorldView *view;
   CombatRoutineLibrary *routines;
   AbilityLibrary *abilities;
   EntitySystem *actionSystem;
   bool executing;
   vec(PostRunTransient) *postRuns;
};

float commandManagerGetAbilityRange(CommandManager *self, Entity *user, StringView cmdID){
   Action *a = actionFromAbilityName(self, user, cmdID);
   if (a){
      ActionRangeComponent *arc = entityGet(ActionRangeComponent)(a);
      if (arc){
         return arc->range;
      }
   }

   return 0.0f;

}

float commandManagerGetAbilitySlotRange(CommandManager *self, Entity *e, size_t slot){
   AbilitySlotsComponent *csc = entityGet(AbilitySlotsComponent)(e);

   if (slot < ABILITY_SLOT_COUNT && csc && csc->slots[slot]){
      return commandManagerGetAbilityRange(self, e, csc->slots[slot]);
   }

   return 0.0f;
}

//determine which coroutine to use from a given action
static Coroutine _updateCommand(CommandManager *self, Action *a){
   ActionTargetPositionComponent *tpc = entityGet(ActionTargetPositionComponent)(a);
   ActionTargetEntityComponent *tec = entityGet(ActionTargetEntityComponent)(a);
   ActionRoutineComponent *rc = entityGet(ActionRoutineComponent)(a);

   if (rc){
      StringView cmdID = rc->routine;

      if (cmdID){
         CombatRoutineGenerator c = combatRoutineLibraryGet(self->routines, cmdID);
         if (!closureIsNull(CombatRoutineGenerator)(&c)){
            return closureCall(&c, self->view, a);
         }
      }         
   }


   //we didnt find a coroutine to run so look and see if we 
   //have movement we can pass to a move command
   if (tpc || tec){
      return createCommandGridMove(a, self->view->managers->gridManager);
   }

   //we aint found shit
   return (Coroutine){ 0 };
}

ImplManagerVTable(CommandManager)

CommandManager *createCommandManager(WorldView *view){
   CommandManager *out = checkedCalloc(1, sizeof(CommandManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(CommandManager);
   out->actionSystem = entitySystemCreate();
   out->routines = combatRoutineLibraryCreate();
   out->abilities = abilityLibraryCreate();
   out->postRuns = vecCreate(PostRunTransient)(&_postRunDestroy);
   out->executing = false;

   buildAllCombatRoutines(out->routines);
   buildAllAbilities(out->abilities);

   return out;
}

void _destroy(CommandManager *self){
   entitySystemDestroy(self->actionSystem);
   combatRoutineLibraryDestroy(self->routines);
   abilityLibraryDestroy(self->abilities);
   vecDestroy(PostRunTransient)(self->postRuns);
   checkedFree(self);
}
void _onDestroy(CommandManager *self, Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   if (cc){
      vecDestroy(ActionPtr)(cc->actions);
   }
}
void _onUpdate(CommandManager *self, Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   TCommandComponent *tcc = entityGet(TCommandComponent)(e);

   if (cc && !tcc){
      if (self->executing){
         vecPushBack(PostRunTransient)(self->postRuns, &(PostRunTransient){.e = e, .add = true});
      }
      else{
         _addNewTCommandComponent(e);
      }
      
   }

   if (tcc && !cc){
      if (self->executing){
         vecPushBack(PostRunTransient)(self->postRuns, &(PostRunTransient){.e = e, .add = false});
      }
      else{
         entityRemove(TCommandComponent)(e);
      }
   }
}

static void _updateEntityCommand(CommandManager *self, Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   TCommandComponent *tcc = entityGet(TCommandComponent)(e);

   while (!vecIsEmpty(ActionPtr)(cc->actions) && !tcc->commandReady){
      tcc->command = _updateCommand(self, *vecBegin(ActionPtr)(cc->actions));
      tcc->commandReady = !closureIsNull(Coroutine)(&tcc->command);
      tcc->runningIndex = 0;
      if (!tcc->commandReady){
         vecRemoveAt(ActionPtr)(cc->actions, 0);
      }
   }
}

static void _updateEntity(CommandManager *self, Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   TCommandComponent *tcc = entityGet(TCommandComponent)(e);

   if (!tcc->commandReady){
      //command not ready, update to try and get a good one
      _updateEntityCommand(self, e);
   }

   //if we got one, run it
   if (tcc->commandReady){
      CoroutineStatus ret = closureCall(&tcc->command, cc->request); 

      //things may havbe gotten shuffled by new commands being created so best to re-retreive here
      cc = entityGet(CommandComponent)(e);
      tcc = entityGet(TCommandComponent)(e);
      
      //we keep executing coroutines bambam until we're out or utnil one returns not finished
      //this allows insta-routines to all be in 1 frame
      while (ret == Finished && !vecIsEmpty(ActionPtr)(cc->actions)){
         tcc->commandReady = false;
         

         //cleanup the current one
         closureDestroy(Coroutine)(&tcc->command);
         vecRemoveAt(ActionPtr)(cc->actions, tcc->runningIndex);  

         //check to see if a paused routine is waiting
         if (!vecIsEmpty(Coroutine)(tcc->pausedCommands)){
            //we have paused actions, pop one off and use it instead of creating a new one
            tcc->command = *vecBack(Coroutine)(tcc->pausedCommands);
            vecPopBack(Coroutine)(tcc->pausedCommands);
            tcc->commandReady = !closureIsNull(Coroutine)(&tcc->command);
         }
         else{
            //update to the next one
            cc->request = Continue;
            _updateEntityCommand(self, e);
         }

         //if theres one avail, run it
         if (tcc->commandReady){
            ret = closureCall(&tcc->command, cc->request);

            //things may havbe gotten shuffled by new commands being created so best to re-retreive here
            cc = entityGet(CommandComponent)(e);
            tcc = entityGet(TCommandComponent)(e);
         }
      }
   }
}

void commandManagerUpdate(CommandManager *self){
   self->executing = true;
   COMPONENT_QUERY(self->view->entitySystem, TCommandComponent, tcc, {
      Entity *e = componentGetParent(tcc, self->view->entitySystem);
      if (!entityIsDead(e)){
         _updateEntity(self, e);
      }      
   });

   vecClear(PostRunTransient)(self->postRuns);
   self->executing = false;
}

static void _actionVDestroy(ActionPtr *self){
   entityDestroy(*self);
}

static CommandComponent *_addCommandComponent(Entity *e){
   COMPONENT_ADD(e, CommandComponent, 
      .actions = vecCreate(ActionPtr)(&_actionVDestroy),
      .request = Continue);
   entityUpdate(e);
   return entityGet(CommandComponent)(e);
}

void entityPauseCommand(Entity *e, Action *cmd){
   TCommandComponent *tcc;

   entityPushFrontCommand(e, cmd);
   tcc = entityGet(TCommandComponent)(e);

   if (tcc && tcc->commandReady){
      closureCall(&tcc->command, Pause);     
      vecPushBack(Coroutine)(tcc->pausedCommands, &tcc->command);

      //weve paused so invalidate the list back at 0
      tcc->runningIndex -= 1;
      tcc->commandReady = false;
   }
}

void entityPushCommand(Entity *e, Action *cmd){
   CommandComponent *cc = entityGet(CommandComponent)(e);

   if (!cc){
      cc = _addCommandComponent(e);      
   }

   COMPONENT_ADD(cmd, ActionUserComponent, e);

   vecPushBack(ActionPtr)(cc->actions, &cmd);
}

void entityPushNextCommand(Entity *e, Action *cmd){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   TCommandComponent *tcc;

   if (!cc){
      cc = _addCommandComponent(e);
   }

   tcc = entityGet(TCommandComponent)(e);

   COMPONENT_ADD(cmd, ActionUserComponent, e);

   vecInsert(ActionPtr)(cc->actions, tcc->runningIndex + 1, &cmd);
}

void entityPushFrontCommand(Entity *e, Action *cmd){
   CommandComponent *cc = entityGet(CommandComponent)(e);

   if (!cc){
      cc = _addCommandComponent(e);
   }

   COMPONENT_ADD(cmd, ActionUserComponent, e);
   vecInsert(ActionPtr)(cc->actions, 0, &cmd);

   if (vecSize(ActionPtr)(cc->actions) > 1){
      entityGet(TCommandComponent)(e)->runningIndex += 1;
   }
   
}

void entityCancelFirstCommand(Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   if (cc){
      if (!vecIsEmpty(ActionPtr)(cc->actions)){
         cc->request = Cancel;
      }
   }
}

void entityForceCancelFirstCommand(Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   if (cc){
      if (!vecIsEmpty(ActionPtr)(cc->actions)){
         cc->request = ForceCancel;
      }
   }
}

void entityCancelAllCommands(Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   if (cc){      
      if (!vecIsEmpty(ActionPtr)(cc->actions)){
         cc->request = Cancel;
         vecResize(ActionPtr)(cc->actions, 1, NULL);
      }
   }
}

void entityForceCancelAllCommands(Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   if (cc){
      if (!vecIsEmpty(ActionPtr)(cc->actions)){
         cc->request = ForceCancel;
         vecResize(ActionPtr)(cc->actions, 1, NULL);
      }
   }
}

void entityClearCommands(Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   if (cc){
      cc->request = Continue;
      vecClear(ActionPtr)(cc->actions);
   }
}

bool entityCommandQueueEmpty(Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   TCommandComponent *tcc = entityGet(TCommandComponent)(e);

   if (cc && tcc){
      size_t actionCount = vecSize(ActionPtr)(cc->actions);
      bool noActionsQueued = actionCount <= 1 + tcc->runningIndex;
      bool noPausedActions = vecIsEmpty(Coroutine)(tcc->pausedCommands);

      return noActionsQueued && noPausedActions;
   }

   return true;
}
bool entityShouldAutoAttack(Entity *e){
   return entityCommandQueueEmpty(e);
}

Action *actionCreateCustom(CommandManager *self){
   return entityCreate(self->actionSystem);
}

Action *actionFromAbilityName(CommandManager *self, Entity *user, StringView name){
   AbilityGenerator gen = abilityLibraryGet(self->abilities, name);
   if (!closureIsNull(AbilityGenerator)(&gen)){
      Action *ret = closureCall(&gen, self->view, user);
      if (ret){
         COMPONENT_ADD(ret, ActionAbilityNameComponent, name);
         return ret;
      }
   }

   return NULL;
}
Action *actionFromAbilitySlot(CommandManager *self, Entity *user, size_t slot){
   AbilitySlotsComponent *asc = entityGet(AbilitySlotsComponent)(user);
   if (asc && slot < ABILITY_SLOT_COUNT && asc->slots[slot]){
      return actionFromAbilityName(self, user, asc->slots[slot]);
   }

   return NULL;
}
Action *actionTargetGridPosition(Action *self, int x, int y){
   COMPONENT_ADD(self, ActionTargetPositionComponent, .x = x, .y = y);
   entityUpdate(self);
   return self;
}
Action *actionTargetEntity(Action *self, Entity *target){
   COMPONENT_ADD(self, ActionTargetEntityComponent, target);
   entityUpdate(self);
   return self;
}

Action *actionSetRange(Action *self, float range){
   COMPONENT_ADD(self, ActionRangeComponent, range);
   entityUpdate(self);
   return self;
}

void actionHelperPushSlot(CommandManager *self, Entity *user, Entity *target, size_t slot){
   Action *a = actionFromAbilitySlot(self, user, slot);
   if (a){
      a = actionTargetEntity(a, target);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushCommand(user, a);
      }
   }
}
void actionHelperPushSlotAtPosition(CommandManager *self, Entity *user, int x, int y, size_t slot){
   Action *a = actionFromAbilitySlot(self, user, slot);
   if (a){
      a = actionTargetGridPosition(a, x, y);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushCommand(user, a);
      }
   }
}
void actionHelperPushAbility(CommandManager *self, Entity *user, Entity *target, StringView ability){
   Action *a = actionFromAbilityName(self, user, ability);
   if (a){
      a = actionTargetEntity(a, target);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushCommand(user, a);
      }
   }
}
void actionHelperPushAbilityAtPosition(CommandManager *self, Entity *user, int x, int y, StringView ability){
   Action *a = actionFromAbilityName(self, user, ability);
   if (a){
      a = actionTargetGridPosition(a, x, y);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushCommand(user, a);
      }
   }
}

void actionHelperPushFrontSlot(CommandManager *self, Entity *user, Entity *target, size_t slot){
   Action *a = actionFromAbilitySlot(self, user, slot);
   if (a){
      a = actionTargetEntity(a, target);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushFrontCommand(user, a);
      }
   }
}
void actionHelperPushFrontSlotAtPosition(CommandManager *self, Entity *user, int x, int y, size_t slot){
   Action *a = actionFromAbilitySlot(self, user, slot);
   if (a){
      a = actionTargetGridPosition(a, x, y);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushFrontCommand(user, a);
      }
   }
}
void actionHelperPushFrontAbility(CommandManager *self, Entity *user, Entity *target, StringView ability){
   Action *a = actionFromAbilityName(self, user, ability);
   if (a){
      a = actionTargetEntity(a, target);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushFrontCommand(user, a);
      }
   }
}
void actionHelperPushFrontAbilityAtPosition(CommandManager *self, Entity *user, int x, int y, StringView ability){
   Action *a = actionFromAbilityName(self, user, ability);
   if (a){
      a = actionTargetGridPosition(a, x, y);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushFrontCommand(user, a);
      }
   }
}

void actionHelperPushMoveToEntity(CommandManager *self, Entity *user, Entity *target, float range){
   Action *a = actionFromAbilityName(self, user, stringIntern("move"));
   if (a){
      a = actionTargetEntity(a, target);
      a = actionSetRange(a, range);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushCommand(user, a);
      }
   }
}
void actionHelperPushMoveToPosition(CommandManager *self, Entity *user, int x, int y, float range){
   Action *a = actionFromAbilityName(self, user, stringIntern("move"));
   if (a){
      a = actionTargetGridPosition(a, x, y);
      a = actionSetRange(a, range);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushCommand(user, a);
      }
   }
}
void actionHelperPushFrontMoveToEntity(CommandManager *self, Entity *user, Entity *target, float range){
   Action *a = actionFromAbilityName(self, user, stringIntern("move"));
   if (a){
      a = actionTargetEntity(a, target);
      a = actionSetRange(a, range);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushFrontCommand(user, a);
      }
   }
}
void actionHelperPushFrontMoveToPosition(CommandManager *self, Entity *user, int x, int y, float range){
   Action *a = actionFromAbilityName(self, user, stringIntern("move"));
   if (a){
      a = actionTargetGridPosition(a, x, y);
      a = actionSetRange(a, range);
      if (!entityGet(ActionInvalidComponent)(a)){
         entityPushFrontCommand(user, a);
      }
   }
}
