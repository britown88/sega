#include "Managers.h"

#include "Managers.h"
#include "ImageManager.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "segautils\IntrusiveHeap.h"
#include "SEGA\App.h"
#include "GridManager.h"

#include <stdio.h>

typedef struct GridNode_t GridNode;

struct GridNode_t{
   QueueNode node;
   byte visited;
   size_t score;
   size_t ID;
   GridNode *parent;
   GridNode *neighbors[4];
   vec(EntityPtr) *entities;
};

static GridNode *_nodeCompareFunc(GridNode *n1, GridNode *n2){
   return n1->score < n2->score ? n1 : n2;
}

#pragma region Dijkstra's'
typedef struct {
   Dijkstras inner;
   size_t destination;
}GridSolver;

static size_t _getNeighbors(GridSolver *self, GridNode *node, GridNode ***outList);
static int _processNeighbor(GridSolver *self, GridNode *current, GridNode *node);
static int _processCurrent(GridSolver *self, GridNode *node);
static void _destroy(GridSolver *self);

static DijkstrasVTable *_getVTable(){
   DijkstrasVTable *out = NULL;
   if (!out){
      out = calloc(1, sizeof(DijkstrasVTable));
      out->getNeighbors = (size_t(*)(Dijkstras*, QueueElem, QueueElem**))&_getNeighbors;
      out->processNeighbor = (int(*)(Dijkstras*, QueueElem, QueueElem))&_processNeighbor;
      out->processCurrent = (int(*)(Dijkstras*, QueueElem))&_processCurrent;
      out->destroy = (void(*)(Dijkstras*))&_destroy;
   }
   return out;
}

size_t _getNeighbors(GridSolver *self, GridNode *node, GridNode ***outList){
   *outList = node->neighbors;
   return 4;
}
int _processNeighbor(GridSolver *self, GridNode *current, GridNode *node){
   if (node && !node->visited){
      size_t newScore = current->score + 1;
      if (newScore < node->score){
         node->parent = current;
         node->score = newScore;
         return true;
      }
   }
   return false;
}
int _processCurrent(GridSolver *self, GridNode *node){
   node->visited = true;

   return node->score == INF || node->ID == self->destination;
}
void _destroy(GridSolver *self){
   priorityQueueDestroy(self->inner.queue);
   checkedFree(self);
}

#pragma endregion

typedef enum {
   IDLE=0,
   MOVING
}GridAction;

typedef struct{
   size_t targetPos;
   GridAction action;
}TGridComponent;

#define TComponentT TGridComponent
#include "Entities\ComponentDeclTransient.h"

struct GridManager_t{
   Manager m;
   EntitySystem *system;

   GridNode table[CELL_COUNT];
};

static void _addNeighbors(GridNode *nodes, size_t current){
   int x, y, i;
   size_t nID[4];
   GridNode *node = nodes + current;
   gridXYFromIndex(node->ID, &x, &y);

   nID[0] = gridIndexFromXY(x, y - 1);
   nID[1] = gridIndexFromXY(x - 1, y);
   nID[2] = gridIndexFromXY(x + 1, y);
   nID[3] = gridIndexFromXY(x, y + 1);

   for (i = 0; i < 4; ++i){
      if (nID[i] < INF){
         node->neighbors[i] = nodes + nID[i];
      }
   }
}

static void _buildTable(GridNode *nodes){
   size_t i;
   for (i = 0; i < CELL_COUNT; ++i){
      nodes[i].ID = i;
      nodes[i].score = INF;
      _addNeighbors(nodes, i);
   }
}

static void _clearTable(GridNode *nodes){
   int i;
   for (i = 0; i < CELL_COUNT; ++i){
      nodes[i].score = INF;
      nodes[i].visited = false;
      nodes[i].parent = NULL;
      queueNodeClear(&nodes[i].node);
   }
}

#pragma region vtable things
static void GridManagerDestroy(GridManager*);
static void GridManagerOnDestroy(GridManager*, Entity*);
static void GridManagerOnUpdate(GridManager*, Entity*);

static ManagerVTable *_createVTable(){
   static ManagerVTable *out = NULL;

   if (!out){
      out = calloc(1, sizeof(ManagerVTable));
      out->destroy = (void(*)(Manager*))&GridManagerDestroy;
      out->onDestroy = (void(*)(Manager*, Entity*))&GridManagerOnDestroy;
      out->onUpdate = (void(*)(Manager*, Entity*))&GridManagerOnUpdate;
   }

   return out;
}

#pragma endregion

GridManager *createGridManager(EntitySystem *system){
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->system = system;
   out->m.vTable = _createVTable();

   _buildTable(out->table);

   return out;
}

void GridManagerDestroy(GridManager *self){
   checkedFree(self);
}

void GridManagerOnDestroy(GridManager *self, Entity *e){}
void GridManagerOnUpdate(GridManager *self, Entity *e){
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   GridComponent *gc = entityGet(GridComponent)(e);

   if (gc){
      if (!tgc){
         //new grid entry
         ADD_NEW_COMPONENT(e, TGridComponent, INF, IDLE);
      }
   }
   else{
      if (tgc){
         //no longer rendered
         entityRemove(TGridComponent)(e);
      }
   }
}

//GridSolution gridManagerSolve(GridManager *self, GridSolveData data, size_t startCell, GridProcessCurrentFunc cFunc, GridProcessNeighborFunc nFunc){
//   int i;
//   PriorityQueue *pq = priorityQueueCreate(offsetof(GridNode, node), (PQCompareFunc)&_nodeCompareFunc);
//   GridSolver *dk = checkedCalloc(1, sizeof(GridSolver));
//   GridNode *result = NULL;
//   GridSolution solution = { INF, INF, INF };
//
//   if (startCell >= 0 && startCell < CELL_COUNT){
//      _clearTable(self->table);
//      self->table[startCell].score = 0;
//
//      for (i = 0; i < CELL_COUNT; ++i){
//         priorityQueuePush(pq, self->table + i);
//      }
//
//      dk->inner.vTable = _getVTable();
//      dk->inner.queue = pq;
//      dk->destination = dest;
//
//      result = dijkstrasRun((Dijkstras*)dk);
//
//      while (result && result->parent && result->parent->parent){
//         result = result->parent;
//      }
//   }
//
//   dijkstrasDestroy((Dijkstras*)dk);
//   return solution;
//}

vec(EntityPtr) *gridManagerEntitiesAt(GridManager *self, size_t index){
   if (index >= 0 && index < CELL_COUNT){
      return self->table[index].entities;
   }
   else {
      return NULL;
   }
}

static size_t derpjkstras(GridManager *self, size_t src, size_t dest){
   int i;
   PriorityQueue *pq = priorityQueueCreate(offsetof(GridNode, node), (PQCompareFunc)&_nodeCompareFunc);
   GridSolver *dk = checkedCalloc(1, sizeof(GridSolver));
   GridNode *result = NULL;

   _clearTable(self->table);
   self->table[src].score = 0;

   for (i = 0; i < CELL_COUNT; ++i){
      priorityQueuePush(pq, self->table + i);
   }

   dk->inner.vTable = _getVTable();
   dk->inner.queue = pq;
   dk->destination = dest;

   result = dijkstrasRun((Dijkstras*)dk);

   while (result && result->parent && result->parent->parent){
      result = result->parent;
   }

   dijkstrasDestroy((Dijkstras*)dk);

   return result->ID;
}

static void _updateEntity(GridManager *self, Entity *e){
   PositionComponent *pc = entityGet(PositionComponent)(e);
   GridComponent *gc = entityGet(GridComponent)(e);
   InterpolationComponent *ic = entityGet(InterpolationComponent)(e);
   TGridComponent *tgc = entityGet(TGridComponent)(e);

   if (pc){
      if (ic){//moving

      }
      else{//idle
         size_t pos = gridIndexFromXY(gc->x, gc->y);
         size_t dest = INF;
         pc->x = GRID_X_POS + gc->x * GRID_RES_SIZE;
         pc->y = GRID_Y_POS + gc->y * GRID_RES_SIZE;

         if (pos == tgc->targetPos || tgc->targetPos == INF){
            size_t newDest = pos;
            while (newDest >= CELL_COUNT || newDest == pos){
               newDest = gridIndexFromXY(appRand(appGet(), 0, TABLE_WIDTH), appRand(appGet(), 0, TABLE_HEIGHT));
            }
            tgc->targetPos = newDest;
         }

         dest = derpjkstras(self, pos, tgc->targetPos);
         gridXYFromIndex(dest, &gc->x, &gc->y);
         ADD_NEW_COMPONENT(e, InterpolationComponent, 
            .destX = GRID_X_POS + gc->x * GRID_RES_SIZE, 
            .destY = GRID_Y_POS + gc->y * GRID_RES_SIZE,
            .time = 0.5);

         entityUpdate(e);
      }
   }
}

void gridManagerUpdate(GridManager *self){
   size_t foo = derpjkstras(self, 3, 75);
   COMPONENT_QUERY(self->system, GridComponent, gc, {
      Entity *e = componentGetParent(gc, self->system);
      _updateEntity(self, e);
   });
}
