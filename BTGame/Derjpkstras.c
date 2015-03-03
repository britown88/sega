#include "Entities\Entities.h"
#include "CoreComponents.h"
#include "GridManager.h"
#include "SEGA\App.h"

typedef struct{
   size_t targetPos;
}TDerpComponent;

#define TComponentT TDerpComponent
#include "Entities\ComponentDeclTransient.h"

typedef struct{
   GridManager *manager;
   size_t destination;
}TestData;

size_t _processNeighbor(TestData *data, GridNodePublic *current, GridNodePublic *neighbor){

   vec(EntityPtr) *entities = gridManagerEntitiesAt(data->manager, neighbor->ID);
   if (entities && !vecIsEmpty(EntityPtr)(entities)){
      return INF;
   }


   return gridNodeGetScore(current) + 1;
}

int _processCurrent(TestData *data, GridNodePublic *current){
   return gridNodeGetScore(current) == INF || current->ID == data->destination;
}

GridSolution solve(GridManager *manager, size_t start, size_t destination){
   
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   TestData data = {manager, destination};
   GridSolution solution;

   closureInit(GridProcessCurrent)(&cFunc, &data, (GridProcessCurrentFunc)&_processCurrent, NULL);
   closureInit(GridProcessNeighbor)(&nFunc, &data, (GridProcessNeighborFunc)&_processNeighbor, NULL);

   return gridManagerSolve(manager, start, cFunc, nFunc);
}


static void _updateEntity(Entity *e, GridComponent *gc, GridManager *manager){
   size_t ID = gridIndexFromXY(gc->x, gc->y);

   PositionComponent *pc = entityGet(PositionComponent)(e);
   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
   TDerpComponent *dc = entityGet(TDerpComponent)(e);

   if (gc && !dc){
      COMPONENT_ADD(e, TDerpComponent, INF);
      dc = entityGet(TDerpComponent)(e);
   }

   if (pc){
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

         solution = solve(manager, ID, dc->targetPos);

         if (solution.totalCost > 0 && solution.totalCost < INF){
            dest = vecBegin(GridSolutionNode)(solution.path)->node;

            COMPONENT_LOCK(GridComponent, newgc, e, {
               gridXYFromIndex(dest, &newgc->x, &newgc->y);
            });
            
            COMPONENT_ADD(e, InterpolationComponent,
               .destX = GRID_X_POS + gc->x * GRID_RES_SIZE,
               .destY = GRID_Y_POS + gc->y * GRID_RES_SIZE,
               .time = 0.5);

            entityUpdate(e);
         }

         
      }
   }




   
   
}

void derjpkstras(EntitySystem *system, GridManager *manager){ 
   COMPONENT_QUERY(system, GridComponent, gc, {
      Entity *e = componentGetParent(gc, system);
      _updateEntity(e, gc, manager);
   });

}
