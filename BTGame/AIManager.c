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

#include <math.h>

typedef struct{
   GridManager *manager;
   size_t targetID;
   Entity *found;
}GridSolvingData;


static float _singleDistance(size_t d0, size_t d1){
   int x0, y0, x1, y1, dist;

   gridXYFromIndex(d0, &x0, &y0);
   gridXYFromIndex(d1, &x1, &y1);

   dist = abs(x0 - x1) + abs(y0 - y1);

   return dist > 1 ? SQRT2 : dist;
}

static float _processNeighbor(GridSolvingData *data, GridNodePublic *current, GridNodePublic *neighbor){
   vec(EntityPtr) *entities = gridManagerEntitiesAt(data->manager, neighbor->ID);
   if (entities && !vecIsEmpty(EntityPtr)(entities)){
      vecForEach(EntityPtr, e, entities, {
         TeamComponent *tc = entityGet(TeamComponent)(*e);
         if (tc && tc->teamID == data->targetID){
            data->found = *e;
            return gridNodeGetScore(current);
         }
      });

      return INFF;
   }
   return gridNodeGetScore(current) + _singleDistance(current->ID, neighbor->ID);
}

static GridNodePublic *_processCurrent(GridSolvingData *data, GridNodePublic *current){
   if (data->found){
      return current;
   }

   return  NULL;
}

static Entity *_solve(GridManager *manager, size_t start, size_t enemyTeamID){
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   GridSolvingData data = { manager, enemyTeamID, NULL };
   GridSolution out;

   closureInit(GridProcessCurrent)(&cFunc, &data, (GridProcessCurrentFunc)&_processCurrent, NULL);
   closureInit(GridProcessNeighbor)(&nFunc, &data, (GridProcessNeighborFunc)&_processNeighbor, NULL);

   out = gridManagerSolve(manager, start, cFunc, nFunc);

   return data.found;
}


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
      Entity *target = _solve(managers->gridManager, gridIndexFromXY(gc->x, gc->y), teamID);
      if (target){
         entityPushCommand(e, createActionCombat(managers->commandManager, 0, target));
      }
   }
}

void AIManagerUpdate(AIManager *self){
   COMPONENT_QUERY(self->view->entitySystem, AIComponent, ai, {
      _updateEntity(self, componentGetParent(ai, self->view->entitySystem));
   });
}