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

#define ComponentT UserComponent
#include "Entities\ComponentImpl.h"

#define ComponentT TargetComponent
#include "Entities\ComponentImpl.h"

#define ComponentT TargetPositionComponent
#include "Entities\ComponentImpl.h"

#define ComponentT RangeComponent
#include "Entities\ComponentImpl.h"

#define VectorTPart ActionPtr
#include "segautils\Vector_Impl.h"

typedef struct{
   Coroutine command; 
   bool commandReady;
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
   EntitySystem *system;
   EntitySystem *actionSystem;
   GridManager *gridManager;
};

static Coroutine _updateCommand(CommandManager *self, Action *a){
   UserComponent *uc = entityGet(UserComponent)(a);
   TargetPositionComponent *tpc = entityGet(TargetPositionComponent)(a);
   TargetComponent *tec = entityGet(TargetComponent)(a);

   if (tpc || tec){
      return createCommandGridMove(a, self->gridManager);
   }

   return (Coroutine){ 0 };
}

ImplManagerVTable(CommandManager)

CommandManager *createCommandManager(EntitySystem *system, GridManager *gridManager){
   CommandManager *out = checkedCalloc(1, sizeof(CommandManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(CommandManager);
   out->actionSystem = entitySystemCreate();
   out->gridManager = gridManager;

   return out;
}

void _destroy(CommandManager *self){
   entitySystemDestroy(self->actionSystem);
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

static bool _coroutineIsGood(Coroutine c){
   return memcmp(&c, &(Coroutine){ 0 }, sizeof(Coroutine)) != 0;
}

static void _updateEntity(CommandManager *self, Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   TCommandComponent *tcc = entityGet(TCommandComponent)(e);

   if (tcc->commandReady){
      CoroutineStatus ret = closureCall(&tcc->command, cc->cancelled);
      if (ret == Finished){
         tcc->commandReady = false;
         cc->cancelled = false;

         closureDestroy(Coroutine)(&tcc->command);
         vecRemoveAt(ActionPtr)(cc->actions, 0);         
      }
   }
   else{//command not ready, keep popping until we find a good one
      while (!vecIsEmpty(ActionPtr)(cc->actions) && !tcc->commandReady){
         tcc->command = _updateCommand(self, *vecBegin(ActionPtr)(cc->actions));
         tcc->commandReady = _coroutineIsGood(tcc->command);
         if (!tcc->commandReady){
            vecRemoveAt(ActionPtr)(cc->actions, 0);
         }
      }
   }
}

void commandManagerUpdate(CommandManager *self){
   COMPONENT_QUERY(self->system, TCommandComponent, tcc, {
      Entity *e = componentGetParent(tcc, self->system);
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

   COMPONENT_ADD(cmd, UserComponent, e);

   vecPushBack(ActionPtr)(cc->actions, &cmd);
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

