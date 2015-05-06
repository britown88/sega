#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "GridManager.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "CoreComponents.h"
#include <math.h>
#include "segautils\Coroutine.h"

typedef struct{
   vec(Coroutine) *list;
   vec(Coroutine) *postCancel;
   Coroutine commands;
   bool cancelled;
}TCommandComponent;

#define TComponentT TCommandComponent
#include "Entities\ComponentDeclTransient.h"


struct CommandManager_t{
   Manager m;
   EntitySystem *system;
};

ImplManagerVTable(CommandManager)

CommandManager *createCommandManager(EntitySystem *system){
   CommandManager *out = checkedCalloc(1, sizeof(CommandManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(CommandManager);

   return out;
}

static void _clearCoroutineList(vec(Coroutine) *list){
   vecForEach(Coroutine, c, list, {
      closureDestroy(Coroutine)(c);
   });
   
}

static void _entityAddCommands(Entity *e){
   TCommandComponent ncc = { 0 };
   ncc.commands = createExecutionList(&ncc.list);
   ncc.postCancel = vecCreate(Coroutine)(NULL);

   entityAdd(TCommandComponent)(e, &ncc);
}

static void _entityDestroyCommands(Entity*e){
   TCommandComponent *cc = entityGet(TCommandComponent)(e);
   if (cc){
      closureDestroy(Coroutine)(&cc->commands);
      _clearCoroutineList(cc->postCancel);
      vecDestroy(Coroutine)(cc->postCancel);
   }
}

static void _updateEntityCommands(Entity *e, TCommandComponent *cc){
   CoroutineStatus status = closureCall(&cc->commands, cc->cancelled);

   if (status == Finished){
      if (cc->cancelled && !vecIsEmpty(Coroutine)(cc->postCancel)){
         //we cancelled and had more commands queued up... copy them over
         vecForEach(Coroutine, cmd, cc->postCancel, {
            vecPushBack(Coroutine)(cc->list, cmd);
         });

         vecClear(Coroutine)(cc->postCancel);
         cc->cancelled = false;
      }
      else{
         //we're all done!
         _entityDestroyCommands(e);
         entityRemove(TCommandComponent)(e);
      }
   }
}

void _destroy(CommandManager *self){
   checkedFree(self);
}
void _onDestroy(CommandManager *self, Entity *e){
   _entityDestroyCommands(e);
}
void _onUpdate(CommandManager *self, Entity *e){}

void commandManagerUpdate(CommandManager *self){
   COMPONENT_QUERY(self->system, TCommandComponent, cc, {
      Entity *e = componentGetParent(cc, self->system);
      _updateEntityCommands(e, cc);
   });
}

void entityPushCommand(Entity *e, Coroutine cmd){
   TCommandComponent *cc = entityGet(TCommandComponent)(e);

   if (!cc){
      _entityAddCommands(e);
      cc = entityGet(TCommandComponent)(e);
   }

   if (cc->cancelled){
      /*command has been canceled but hasnt finished yet and
       we dont know if an iteration of cancelled routine has been done so
       we store new routines in a postcancel queue
       which gets copied over when a cancelled queue finishes*/
      vecPushBack(Coroutine)(cc->postCancel, &cmd);
   }
   else{
      vecPushBack(Coroutine)(cc->list, &cmd);
   }
}

void entityCancelCommands(Entity *e){
   TCommandComponent *cc = entityGet(TCommandComponent)(e);
   if (cc){
      cc->cancelled = true;
   }
}

void entityClearCommands(Entity *e){
   TCommandComponent *cc = entityGet(TCommandComponent)(e);
   if (cc){
      _entityDestroyCommands(e);
      entityRemove(TCommandComponent)(e);
      _entityAddCommands(e);
   }
}

