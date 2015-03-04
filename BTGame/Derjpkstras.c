#include "Entities\Entities.h"
#include "CoreComponents.h"
#include "GridManager.h"
#include "SEGA\App.h"
#include "segautils\StandardVectors.h"
#include <math.h>

typedef struct{
   size_t targetPos;
}TDerpComponent;

#define TComponentT TDerpComponent
#include "Entities\ComponentDeclTransient.h"

typedef struct{
   GridManager *manager;
   size_t destination;

   size_t lowestHeuristic;
   GridNodePublic *closestNode;
}TestData;

size_t _processNeighbor(TestData *data, GridNodePublic *current, GridNodePublic *neighbor){
   vec(EntityPtr) *entities = gridManagerEntitiesAt(data->manager, neighbor->ID);
   if (entities && !vecIsEmpty(EntityPtr)(entities)){
      return INF;
   }
   return gridNodeGetScore(current) + 1;
}

GridNodePublic *_processCurrent(TestData *data, GridNodePublic *current){
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

GridSolution solve(GridManager *manager, size_t start, size_t destination){
   
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   TestData data = {manager, destination, INF, NULL};

   closureInit(GridProcessCurrent)(&cFunc, &data, (GridProcessCurrentFunc)&_processCurrent, NULL);
   closureInit(GridProcessNeighbor)(&nFunc, &data, (GridProcessNeighborFunc)&_processNeighbor, NULL);

   GridSolution solution = gridManagerSolve(manager, start, cFunc, nFunc);

   if (data.lowestHeuristic != solution.totalCost){
      //update solution
   }

   return solution;
}


static void _updateEntity(Entity *e, GridManager *manager){
   

   GridComponent *gc = entityGet(GridComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
   TDerpComponent *dc = entityGet(TDerpComponent)(e);

   if (gc && !dc){
      COMPONENT_ADD(e, TDerpComponent, INF);
      dc = entityGet(TDerpComponent)(e);
   }

   if (gc && pc){
      if (ic){//moving

      }
      else{//idle
         GridSolution solution;
         size_t pos = gridIndexFromXY(gc->x, gc->y);
         size_t dest = INF;
         pc->x = GRID_X_POS + gc->x * GRID_RES_SIZE;
         pc->y = GRID_Y_POS + gc->y * GRID_RES_SIZE;

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

void derjpkstras(EntitySystem *system, GridManager *manager){ 
   COMPONENT_QUERY(system, WanderComponent, c, {
      Entity *e = componentGetParent(c, system);
      _updateEntity(e, manager);
   });

}
