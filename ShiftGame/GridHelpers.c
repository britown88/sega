#include "GridHelpers.h"
#include "CoreComponents.h"
#include "Combat.h"
#include <math.h>

typedef struct{
   GridManager *manager;
   size_t targetID;
   float maxRange;
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
   float newScore = gridNodeGetScore(current) + _singleDistance(current->ID, neighbor->ID);

   if (data->maxRange < INFF && newScore > data->maxRange){
      return INFF;
   }

   if (entities && !vecIsEmpty(EntityPtr)(entities)){
      vecForEach(EntityPtr, e, entities, {
         TeamComponent *tc = entityGet(TeamComponent)(*e);
         if (!entityIsDead(*e) && tc && tc->teamID == data->targetID){
            data->found = *e;
            return gridNodeGetScore(current);
         }
      });

      return INFF;
   }

   return newScore;
}

static GridNodePublic *_processCurrent(GridSolvingData *data, GridNodePublic *current){
   if (data->found){
      return current;
   }

   return  NULL;
}

Entity *gridFindClosestEntity(GridManager *manager, size_t start, size_t enemyTeamID, float maxRange){
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   GridSolvingData data = { manager, enemyTeamID, maxRange, NULL };
   GridSolution out;

   closureInit(GridProcessCurrent)(&cFunc, &data, (GridProcessCurrentFunc)&_processCurrent, NULL);
   closureInit(GridProcessNeighbor)(&nFunc, &data, (GridProcessNeighborFunc)&_processNeighbor, NULL);

   out = gridManagerSolve(manager, start, cFunc, nFunc);

   return data.found;
}
