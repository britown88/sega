#include "Managers.h"

#include "Managers.h"
#include "ImageLibrary.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "segautils\IntrusiveHeap.h"
#include "SEGA\App.h"
#include "GridManager.h"
#include "segautils\StandardVectors.h"
#include "WorldView.h"

#include <stdio.h>
#include <math.h>

#define ClosureTPart CLOSURE_NAME(GridProcessNeighbor)
#include "segautils\Closure_Impl.h"

#define ClosureTPart CLOSURE_NAME(GridProcessCurrent)
#include "segautils\Closure_Impl.h"

#define VectorTPart GridSolutionNode
#include "segautils\Vector_Impl.h"

typedef struct GridNode_t GridNode;

#define NEIGHBOR_COUNT 8

struct GridNode_t{
   GridNodePublic data;
   QueueNode node;
   byte visited;
   GridNode *parent;
   GridNode *neighbors[NEIGHBOR_COUNT];
   float score;
};

float gridNodeGetScore(GridNodePublic *self){
   return ((GridNode*)self)->score;
}

static GridNode *_nodeCompareFunc(GridNode *n1, GridNode *n2){
   return n1->score < n2->score ? n1 : n2;
}

#pragma region Dijkstra's'
typedef struct {
   Dijkstras inner;
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   GridNode *solutionNode;
}GridSolver;

static size_t _solverGetNeighbors(GridSolver *self, GridNode *node, GridNode ***outList);
static int _solverProcessNeighbor(GridSolver *self, GridNode *current, GridNode *node);
static int _solverProcessCurrent(GridSolver *self, GridNode *node, bool last);
static void _solverDestroy(GridSolver *self);

static DijkstrasVTable *_getVTable(){
   DijkstrasVTable *out = NULL;
   if (!out){
      out = calloc(1, sizeof(DijkstrasVTable));
      out->getNeighbors = (size_t(*)(Dijkstras*, QueueElem, QueueElem**))&_solverGetNeighbors;
      out->processNeighbor = (int(*)(Dijkstras*, QueueElem, QueueElem))&_solverProcessNeighbor;
      out->processCurrent = (int(*)(Dijkstras*, QueueElem, bool))&_solverProcessCurrent;
      out->destroy = (void(*)(Dijkstras*))&_solverDestroy;
   }
   return out;
}

size_t _solverGetNeighbors(GridSolver *self, GridNode *node, GridNode ***outList){
   *outList = node->neighbors;
   return NEIGHBOR_COUNT;
}
int _solverProcessNeighbor(GridSolver *self, GridNode *current, GridNode *node){
   if (node && !node->visited){
      float newScore = closureCall(&self->nFunc, &current->data, &node->data);
      if (newScore < node->score){
         node->parent = current;
         node->score = newScore;
         return true;
      }
   }
   return false;
}
int _solverProcessCurrent(GridSolver *self, GridNode *node, bool last){
   node->visited = true;
   GridNode *solution = (GridNode*)closureCall(&self->cFunc, &node->data);
   if (solution){
      self->solutionNode = solution;
      return true;
   }

   return false;
}
void _solverDestroy(GridSolver *self){
   priorityQueueDestroy(self->inner.queue);
   checkedFree(self);
}

#pragma endregion


typedef struct{
   vec(size_t) *occupyingNodes;
}TGridComponent;

#define TComponentT TGridComponent
#include "Entities\ComponentDeclTransient.h"

struct GridManager_t{
   Manager m;
   WorldView *view;

   GridNode table[CELL_COUNT];
   vec(GridSolutionNode) *solutionMap;

   PriorityQueue *pq;// = priorityQueueCreate(offsetof(GridNode, node), (PQCompareFunc)&_nodeCompareFunc);
   GridSolver *solver;// = checkedCalloc(1, sizeof(GridSolver));
};

static void _addNeighbors(GridNode *nodes, size_t current){
   int x, y, i;
   size_t nID[NEIGHBOR_COUNT];
   GridNode *node = nodes + current;
   gridXYFromIndex(node->data.ID, &x, &y);

   nID[0] = gridIndexFromXY(x, y - 1);
   nID[1] = gridIndexFromXY(x - 1, y);
   nID[2] = gridIndexFromXY(x + 1, y);
   nID[3] = gridIndexFromXY(x, y + 1);

   nID[4] = gridIndexFromXY(x - 1, y - 1);
   nID[5] = gridIndexFromXY(x + 1, y + 1);
   nID[6] = gridIndexFromXY(x + 1, y - 1);
   nID[7] = gridIndexFromXY(x - 1, y + 1);

   for (i = 0; i < NEIGHBOR_COUNT; ++i){
      if (nID[i] < INF){
         node->neighbors[i] = nodes + nID[i];
      }
   }
}

static void _buildTable(GridNode *nodes){
   size_t i;
   for (i = 0; i < CELL_COUNT; ++i){
      nodes[i].data.ID = i;
      nodes[i].score = INFF;
      nodes[i].data.entities = vecCreate(EntityPtr)(NULL);
      _addNeighbors(nodes, i);

   }
}

static void _clearTable(GridNode *nodes){
   int i;
   for (i = 0; i < CELL_COUNT; ++i){
      nodes[i].score = INFF;
      nodes[i].visited = false;
      nodes[i].parent = NULL;
      queueNodeClear(&nodes[i].node);
   }
}

static void _gridAddEntity(GridManager *self, Entity *e, size_t newPos){
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   if (newPos < CELL_COUNT){
      vecPushBack(size_t)(tgc->occupyingNodes, &newPos);
      vecPushBack(EntityPtr)(gridManagerEntitiesAt(self, newPos), &e);
   }
}

static void _gridRemoveEntity(GridManager *self, Entity *e, size_t oldPos){
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   if (tgc){
      vec(EntityPtr) *entitiesAtOld = gridManagerEntitiesAt(self, oldPos);
      vec(size_t) *nodes = tgc->occupyingNodes;

      vecRemove(size_t)(nodes, &oldPos);

      if (entitiesAtOld){
         vecRemove(EntityPtr)(entitiesAtOld, &e);
      }
   }
}

static void _gridMoveEntity(GridManager *self, Entity *e, size_t oldPos, size_t newPos){

   if (oldPos != newPos && newPos < CELL_COUNT){
      _gridRemoveEntity(self, e, oldPos);
      _gridAddEntity(self, e, newPos);
   }
}

static void _gridComponentUpdate(GridManager *self, Entity *e, GridComponent *oldGC){
   GridComponent *gc = entityGet(GridComponent)(e);
   
   size_t oldPos = gridIndexFromXY(oldGC->x, oldGC->y);
   size_t newPos = gridIndexFromXY(gc->x, gc->y);

   _gridMoveEntity(self, e, oldPos, newPos);
}

static void _registerUpdateDelegate(GridManager *self, EntitySystem *system){
   ComponentUpdate update;
   
   closureInit(ComponentUpdate)(&update, self, (ComponentUpdateFunc)&_gridComponentUpdate, NULL);
   compRegisterUpdateDelegate(GridComponent)(system, update);
}

ImplManagerVTable(GridManager)

GridManager *createGridManager(WorldView *view){
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(GridManager);
   out->solutionMap = vecCreate(GridSolutionNode)(NULL);

   //destroyed by solver destroy
   out->pq = priorityQueueCreate(offsetof(GridNode, node), (PQCompareFunc)&_nodeCompareFunc);

   out->solver = checkedCalloc(1, sizeof(GridSolver));
   out->solver->inner.vTable = _getVTable();
   out->solver->inner.queue = out->pq;

   _buildTable(out->table);

   _registerUpdateDelegate(out, view->entitySystem);

   return out;
}

void _destroy(GridManager *self){
   size_t i;
   for (i = 0; i < CELL_COUNT; ++i){
      vecDestroy(EntityPtr)(self->table[i].data.entities);
   }
   vecDestroy(GridSolutionNode)(self->solutionMap);
   dijkstrasDestroy((Dijkstras*)self->solver);
   checkedFree(self);
}

void _onDestroy(GridManager *self, Entity *e){
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   if (tgc){
      vecForEach(size_t, node, tgc->occupyingNodes, {
         vec(EntityPtr) *entitiesAtOld = gridManagerEntitiesAt(self, *node);
         if (entitiesAtOld){
            vecRemove(EntityPtr)(entitiesAtOld, &e);
         }
      });
      vecDestroy(size_t)(tgc->occupyingNodes);
   }
}

static void _removeEntityFromNode(GridManager *self, Entity *e, size_t node){
   vec(EntityPtr) *entitiesAtOld = gridManagerEntitiesAt(self, node);
   if (entitiesAtOld){
      vecRemove(EntityPtr)(entitiesAtOld, &e);
   }
}

void _onUpdate(GridManager *self, Entity *e){
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   GridComponent *gc = entityGet(GridComponent)(e);

   if (gc){
      if (!tgc){
         PositionComponent *pc = entityGet(PositionComponent)(e);
         if (pc){
            pc->x = GRID_X_POS + gc->x * GRID_RES_SIZE;
            pc->y = GRID_Y_POS + gc->y * GRID_RES_SIZE;
         }

         //new grid entry
         COMPONENT_ADD(e, TGridComponent, vecCreate(size_t)(NULL));
         _gridAddEntity(self, e, gridIndexFromXY(gc->x, gc->y));
      }
   }
   else{
      if (tgc){
         //no longer on grid, remove from occupying nodes
         vecForEach(size_t, node, tgc->occupyingNodes, {
            _removeEntityFromNode(self, e, *node);
         });
         vecDestroy(size_t)(tgc->occupyingNodes);
         entityRemove(TGridComponent)(e);
      }
   }
}

GridSolution gridManagerSolve(GridManager *self, size_t startCell, GridProcessCurrent cFunc, GridProcessNeighbor nFunc){
   GridSolution solution = { INFF, INF, NULL };

   if (startCell < CELL_COUNT){
      int i;
      GridNode *result = NULL;
      _clearTable(self->table);
      self->table[startCell].score = 0;

      priorityQueueClear(self->pq);

      for (i = 0; i < CELL_COUNT; ++i){
         priorityQueuePush(self->pq, self->table + i);
      }
      
      self->solver->cFunc = cFunc;
      self->solver->nFunc = nFunc;

      dijkstrasRun((Dijkstras*)self->solver);

      result = self->solver->solutionNode;

      if (result){
         solution.solutionCell = result->data.ID;
         solution.totalCost = result->score;

         vecClear(GridSolutionNode)(self->solutionMap);

         while (result && result->parent){
            GridSolutionNode resoluteNode = { .node = result->data.ID };
            vecPushBack(GridSolutionNode)(self->solutionMap, &resoluteNode);
            result = result->parent;
         }

         vecReverse(GridSolutionNode)(self->solutionMap);
         solution.path = self->solutionMap;
      }
   }

   return solution;
}

vec(EntityPtr) *gridManagerEntitiesAt(GridManager *self, size_t index){
   if (index < CELL_COUNT){
      return self->table[index].data.entities;
   }
   else {
      return NULL;
   }
}

size_t gridIndexFromScreenXY(int x, int y){
   return gridIndexFromXY(
      (x - GRID_X_POS) / GRID_RES_SIZE,
      (y - GRID_Y_POS) / GRID_RES_SIZE);
}

void gridXYFromScreenXY(int x, int y, int *gx, int *gy){
   gridXYFromIndex(gridIndexFromScreenXY(x, y), gx, gy);
}

size_t gridIndexFromXY(int x, int y){
   if (x < 0 || x >= TABLE_WIDTH || y < 0 || y >= TABLE_HEIGHT){
      return INF;
   }

   return TABLE_WIDTH * y + x;
}

void gridXYFromIndex(size_t index, int*x, int*y){
   *x = index % TABLE_WIDTH;
   *y = index / TABLE_WIDTH;
}

void screenPosFromGridXY(int gx, int gy, int *x, int *y){
   *x = GRID_X_POS + gx * GRID_RES_SIZE;
   *y = GRID_Y_POS + gy * GRID_RES_SIZE;
}

void screenPosFromGridIndex(size_t index, int *x, int *y){
   int gx, gy;
   gridXYFromIndex(index, &gx, &gy);
   screenPosFromGridXY(gx, gy, x, y);
}

int gridDistance(Entity *user, Entity *target){
   GridComponent *gc0 = entityGet(GridComponent)(user);
   GridComponent *gc1 = entityGet(GridComponent)(target);

   if (gc0 && gc1){
      return abs(gc0->x - gc1->x) + abs(gc0->y - gc1->y);
   }

   return 0;
}


