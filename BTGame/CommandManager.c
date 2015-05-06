#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "GridManager.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "CoreComponents.h"
#include <math.h>

typedef struct{
   size_t targetPos;
}TGridTargetComponent;

#define TComponentT TGridTargetComponent
#include "Entities\ComponentDeclTransient.h"

typedef struct{
   GridManager *manager;
   size_t destination;

   size_t lowestHeuristic;
   GridNodePublic *closestNode;
}TestData;

static size_t _processNeighbor(TestData *data, GridNodePublic *current, GridNodePublic *neighbor){
   vec(EntityPtr) *entities = gridManagerEntitiesAt(data->manager, neighbor->ID);
   if (entities && !vecIsEmpty(EntityPtr)(entities)){
      return INF;
   }
   return gridNodeGetScore(current) + 1;
}

static GridNodePublic *_processCurrent(TestData *data, GridNodePublic *current){
   size_t currentScore = gridNodeGetScore(current);
   size_t h;
   int x1 = 0, x2 = 0, y1 = 0, y2 = 0;

   if (gridNodeGetScore(current) == INF){
      //cant get to destination, use closest
      return data->closestNode;
   }

   if (current->ID == data->destination){
      //destination found
      return current;
   }

   //update heuristic
   gridXYFromIndex(current->ID, &x1, &y1);
   gridXYFromIndex(data->destination, &x2, &y2);
   h = abs(x1 - x2) + abs(y1 - y2);
   if (h < data->lowestHeuristic){
      data->lowestHeuristic = h;
      data->closestNode = current;
   }

   return  NULL;
}

static GridSolution solve(GridManager *manager, size_t start, size_t destination){

   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   TestData data = { manager, destination, INF, NULL };

   closureInit(GridProcessCurrent)(&cFunc, &data, (GridProcessCurrentFunc)&_processCurrent, NULL);
   closureInit(GridProcessNeighbor)(&nFunc, &data, (GridProcessNeighborFunc)&_processNeighbor, NULL);

   return gridManagerSolve(manager, start, cFunc, nFunc);
}


static void _updateEntity(Entity *e, GridManager *manager){
   GridComponent *gc = entityGet(GridComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
   TGridTargetComponent *dc = entityGet(TGridTargetComponent)(e);

   if (gc && !dc){
      COMPONENT_ADD(e, TGridTargetComponent, INF);
      dc = entityGet(TGridTargetComponent)(e);
   }

   if (gc && pc){
      if (ic){//moving

      }
      else{//idle
         GridSolution solution;
         size_t pos = gridIndexFromXY(gc->x, gc->y);
         size_t dest = INF;

         if (pos == dc->targetPos || dc->targetPos == INF){
            size_t newDest = pos;
            while (newDest >= CELL_COUNT || newDest == pos){
               newDest = gridIndexFromXY(appRand(appGet(), 0, TABLE_WIDTH), appRand(appGet(), 0, TABLE_HEIGHT));
            }
            dc->targetPos = newDest;
         }

         solution = solve(manager, pos, dc->targetPos);
         if (solution.totalCost == 0){
            dc->targetPos = solution.solutionCell;
         }

         if (solution.totalCost > 0){
            dest = vecBegin(GridSolutionNode)(solution.path)->node;

            COMPONENT_LOCK(GridComponent, newgc, e, {
               gridXYFromIndex(dest, &newgc->x, &newgc->y);
            });

            COMPONENT_ADD(e, InterpolationComponent,
               .destX = GRID_X_POS + gc->x * GRID_RES_SIZE,
               .destY = GRID_Y_POS + gc->y * GRID_RES_SIZE,
               .time = 0.50);

            entityUpdate(e);
         }


      }
   }
}

static void derjpkstras(EntitySystem *system, GridManager *manager){
   COMPONENT_QUERY(system, WanderComponent, c, {
      Entity *e = componentGetParent(c, system);
      _updateEntity(e, manager);
   });

}


struct CommandManager_t{
   Manager m;
   EntitySystem *system;

   GridManager *grid;
};

ImplManagerVTable(CommandManager)

CommandManager *createCommandManager(EntitySystem *system, GridManager *grid){
   CommandManager *out = checkedCalloc(1, sizeof(CommandManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(CommandManager);
   out->grid = grid;

   return out;
}

void _destroy(CommandManager *self){
   checkedFree(self);
}
void _onDestroy(CommandManager *self, Entity *e){}
void _onUpdate(CommandManager *self, Entity *e){}

void commandManagerUpdate(CommandManager *self){
}