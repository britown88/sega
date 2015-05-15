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

#define ComponentT ActionUserComponent
#include "Entities\ComponentImpl.h"

#define ComponentT ActionTargetEntityComponent
#include "Entities\ComponentImpl.h"

#define ComponentT ActionTargetPositionComponent
#include "Entities\ComponentImpl.h"

#define ComponentT ActionRangeComponent
#include "Entities\ComponentImpl.h"

#define ComponentT ActionCombatComponent
#include "Entities\ComponentImpl.h"

#define VectorTPart ActionPtr
#include "segautils\Vector_Impl.h"

typedef struct{
   Coroutine command; 
   bool commandReady;
   size_t runningIndex;//0 by  default
}TCommandComponent;

static void TCommandComponentDestroy(TCommandComponent *self){
   if (self->commandReady){
      closureDestroy(Coroutine)(&self->command);
   }
}

#define COMP_DESTROY_FUNC TCommandComponentDestroy
#define TComponentT TCommandComponent
#include "Entities\ComponentDeclTransient.h"


struct CommandManager_t{
   Manager m;
   WorldView *view;
   CombatRoutineLibrary *routines;
   EntitySystem *actionSystem;
};

//determine which coroutine to use from a given action
static Coroutine _updateCommand(CommandManager *self, Action *a){
   ActionTargetPositionComponent *tpc = entityGet(ActionTargetPositionComponent)(a);
   ActionTargetEntityComponent *tec = entityGet(ActionTargetEntityComponent)(a);
   ActionCombatComponent *cc = entityGet(ActionCombatComponent)(a);

   if (cc && cc->slot < COMBAT_SLOT_COUNT){
      ActionUserComponent *uc = entityGet(ActionUserComponent)(a);
      CombatSlotsComponent *csc = entityGet(CombatSlotsComponent)(uc->user);
      if (csc && csc->slots[cc->slot]){
         CombatRoutineGenerator c = combatRoutineLibraryGet(self->routines, csc->slots[cc->slot]);
         if (!closureIsNull(CombatRoutineGenerator)(&c)){

            //...*ahem*
            // action has a slot and a user, that user has combat slots,
            // the slot selected by the action is assigned in the user,
            // that slot name is a valid coroutine
            // so...return it, otherwise drop through so move is default
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

   buildAllCombatRoutines(out->routines);

   return out;
}

void _destroy(CommandManager *self){
   entitySystemDestroy(self->actionSystem);
   combatRoutineLibraryDestroy(self->routines);
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
      COMPONENT_ADD(e, TCommandComponent, 0);
   }

   if (tcc && !cc){
      entityRemove(TCommandComponent)(e);
   }
}

Action *commandManagerCreateAction(CommandManager *self){
   return entityCreate(self->actionSystem);
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
      CoroutineStatus ret = closureCall(&tcc->command, cc->cancelled);      
      
      //we keep executing coroutines bambam until we're out or utnil one returns not finished
      //this allows insta-routines to all be in 1 frame
      while (ret == Finished && !vecIsEmpty(ActionPtr)(cc->actions)){
         tcc->commandReady = false;
         cc->cancelled = false;

         //cleanup the current one
         closureDestroy(Coroutine)(&tcc->command);
         vecRemoveAt(ActionPtr)(cc->actions, tcc->runningIndex);   

         //update to the next one
         _updateEntityCommand(self, e);

         //if theres one avail, run it
         if (tcc->commandReady){
            ret = closureCall(&tcc->command, cc->cancelled);
         }
      }
   }
}

void commandManagerUpdate(CommandManager *self){
   COMPONENT_QUERY(self->view->entitySystem, TCommandComponent, tcc, {
      Entity *e = componentGetParent(tcc, self->view->entitySystem);
      _updateEntity(self, e);
   });
}

static void _actionVDestroy(ActionPtr *self){
   entityDestroy(*self);
}

static CommandComponent *_addCommandComponent(Entity *e){
   COMPONENT_ADD(e, CommandComponent, 
      .actions = vecCreate(ActionPtr)(&_actionVDestroy),
      .cancelled = false);
   entityUpdate(e);
   return entityGet(CommandComponent)(e);
}

void entityPushCommand(Entity *e, Action *cmd){
   CommandComponent *cc = entityGet(CommandComponent)(e);

   if (!cc){
      cc = _addCommandComponent(e);      
   }

   COMPONENT_ADD(cmd, ActionUserComponent, e);

   vecPushBack(ActionPtr)(cc->actions, &cmd);
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

void entityCancelCommands(Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   if (cc){      
      if (!vecIsEmpty(ActionPtr)(cc->actions)){
         cc->cancelled = true;
         vecResize(ActionPtr)(cc->actions, 1, NULL);
      }
   }
}

void entityClearCommands(Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   if (cc){
      cc->cancelled = false;
      vecClear(ActionPtr)(cc->actions);
   }
}

