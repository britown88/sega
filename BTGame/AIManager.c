#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"
#include "GridManager.h"
#include "Combat.h"
#include "GridHelpers.h"

#include <math.h>



struct AIManager_t{
   Manager m;
   WorldView *view;
};


ImplManagerVTable(AIManager)

AIManager *createAIManager(WorldView *view){
   AIManager *out = checkedCalloc(1, sizeof(AIManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(AIManager);


   return out;
}

void _destroy(AIManager *self){

   checkedFree(self);
}
void _onDestroy(AIManager *self, Entity *e){}
void _onUpdate(AIManager *self, Entity *e){}

static void _updateEntity(AIManager *self, Entity *e){
   CommandComponent *cc = entityGet(CommandComponent)(e);
   GridComponent *gc = entityGet(GridComponent)(e);
   TeamComponent *tc = entityGet(TeamComponent)(e);
   BTManagers *managers = self->view->managers;
   if (!gc || !tc || entityIsDead(e)){
      return;
   }

   if (!cc || vecIsEmpty(ActionPtr)(cc->actions)){
      size_t teamID = !tc->teamID;
      Entity *target = gridFindClosestEntity(managers->gridManager, gridIndexFromXY(gc->x, gc->y), teamID, INFF);
      if (target){
         entityPushCommand(e, createActionCombatSlot(managers->commandManager, 0, target));
      }
   }
}

void AIManagerUpdate(AIManager *self){
   COMPONENT_QUERY(self->view->entitySystem, AIComponent, ai, {
      _updateEntity(self, componentGetParent(ai, self->view->entitySystem));
   });
}