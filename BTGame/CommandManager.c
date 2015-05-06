#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "GridManager.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "CoreComponents.h"
#include <math.h>
#include "segautils\Coroutine.h"



//typedef struct{
//   GridManager *manager;
//   size_t destination;
//
//   size_t lowestHeuristic;
//   GridNodePublic *closestNode;
//}TestData;
//
//static size_t _processNeighbor(TestData *data, GridNodePublic *current, GridNodePublic *neighbor){
//   vec(EntityPtr) *entities = gridManagerEntitiesAt(data->manager, neighbor->ID);
//   if (entities && !vecIsEmpty(EntityPtr)(entities)){
//      return INF;
//   }
//   return gridNodeGetScore(current) + 1;
//}
//
//static GridNodePublic *_processCurrent(TestData *data, GridNodePublic *current){
//   size_t currentScore = gridNodeGetScore(current);
//   size_t h;
//   int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
//
//   if (gridNodeGetScore(current) == INF){
//      //cant get to destination, use closest
//      return data->closestNode;
//   }
//
//   if (current->ID == data->destination){
//      //destination found
//      return current;
//   }
//
//   //update heuristic
//   gridXYFromIndex(current->ID, &x1, &y1);
//   gridXYFromIndex(data->destination, &x2, &y2);
//   h = abs(x1 - x2) + abs(y1 - y2);
//   if (h < data->lowestHeuristic){
//      data->lowestHeuristic = h;
//      data->closestNode = current;
//   }
//
//   return  NULL;
//}
//
//static GridSolution solve(GridManager *manager, size_t start, size_t destination){
//
//   GridProcessCurrent cFunc;
//   GridProcessNeighbor nFunc;
//   TestData data = { manager, destination, INF, NULL };
//
//   closureInit(GridProcessCurrent)(&cFunc, &data, (GridProcessCurrentFunc)&_processCurrent, NULL);
//   closureInit(GridProcessNeighbor)(&nFunc, &data, (GridProcessNeighborFunc)&_processNeighbor, NULL);
//
//   return gridManagerSolve(manager, start, cFunc, nFunc);
//}
//
//
//static void _updateEntity(Entity *e, GridManager *manager){
//   GridComponent *gc = entityGet(GridComponent)(e);
//   PositionComponent *pc = entityGet(PositionComponent)(e);
//   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
//   TGridTargetComponent *dc = entityGet(TGridTargetComponent)(e);
//
//   if (gc && !dc){
//      COMPONENT_ADD(e, TGridTargetComponent, INF);
//      dc = entityGet(TGridTargetComponent)(e);
//   }
//
//   if (gc && pc){
//      if (ic){//moving
//
//      }
//      else{//idle
//         GridSolution solution;
//         size_t pos = gridIndexFromXY(gc->x, gc->y);
//         size_t dest = INF;
//
//         if (pos == dc->targetPos || dc->targetPos == INF){
//            size_t newDest = pos;
//            while (newDest >= CELL_COUNT || newDest == pos){
//               newDest = gridIndexFromXY(appRand(appGet(), 0, TABLE_WIDTH), appRand(appGet(), 0, TABLE_HEIGHT));
//            }
//            dc->targetPos = newDest;
//         }
//
//         solution = solve(manager, pos, dc->targetPos);
//         if (solution.totalCost == 0){
//            dc->targetPos = solution.solutionCell;
//         }
//
//         if (solution.totalCost > 0){
//            dest = vecBegin(GridSolutionNode)(solution.path)->node;
//
//            COMPONENT_LOCK(GridComponent, newgc, e, {
//               gridXYFromIndex(dest, &newgc->x, &newgc->y);
//            });
//
//            COMPONENT_ADD(e, InterpolationComponent,
//               .destX = GRID_X_POS + gc->x * GRID_RES_SIZE,
//               .destY = GRID_Y_POS + gc->y * GRID_RES_SIZE,
//               .time = 0.50);
//
//            entityUpdate(e);
//         }
//
//
//      }
//   }
//}
//
//static void derjpkstras(EntitySystem *system, GridManager *manager){
//   COMPONENT_QUERY(system, WanderComponent, c, {
//      Entity *e = componentGetParent(c, system);
//      _updateEntity(e, manager);
//   });
//
//}

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

