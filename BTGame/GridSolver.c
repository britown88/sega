#include "Managers.h"
#include "GridManager.h"
#include "GridSolver.h"
#include "CoreComponents.h"

#include "segautils/IntrusiveHeap.h"

#define DK_SEARCH_RADIUS 10
#define DK_NEIGHBOR_COUNT 4

typedef struct GridNode_t GridNode;

#pragma pack(push, 1)
//used for dijkstra's solves
struct GridNode_t {
   GridNodePublic data;
   QueueNode node;
   GridNode *parent;//the observing node
   GridNode *neighbors[DK_NEIGHBOR_COUNT];
   byte visited;//for node generation
   size_t index;//index in the search area table (useful to check neighbors)
   float score;
};
#pragma pack(pop)

float gridNodeGetScore(GridNodePublic *self) {
   return ((GridNode*)self)->score;
}

static GridNode *_nodeCompareFunc(GridNode *n1, GridNode *n2) {
   return n1->score < n2->score ? n1 : n2;
}

#define ClosureTPart CLOSURE_NAME(GridProcessNeighbor)
#include "segautils\Closure_Impl.h"

#define ClosureTPart CLOSURE_NAME(GridProcessCurrent)
#include "segautils\Closure_Impl.h"

#define VectorTPart GridSolutionNode
#include "segautils\Vector_Impl.h"

#define VectorT GridNode
#include "segautils/Vector_Create.h"

struct GridSolver_t{
   Dijkstras inner;
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   GridNode *solutionNode;

   GridManager *manager;

   vec(GridNode) *solvingTable;

   //precompute out a few things here for ease of grabbing
   Recti solutionSearchArea;
   int solveWidth, solveHeight;
   size_t solveCount;

   vec(GridSolutionNode) *solutionMap;
   vec(EntityPtr) *eList;
};

static size_t _solverGetNeighbors(GridSolver *self, GridNode *node, GridNode ***outList);
static int _solverProcessNeighbor(GridSolver *self, GridNode *current, GridNode *node);
static int _solverProcessCurrent(GridSolver *self, GridNode *node, bool last);
static void _solverDestroy(GridSolver *self);

static DijkstrasVTable *_getSolverVTable() {
   DijkstrasVTable *out = NULL;
   if (!out) {
      out = calloc(1, sizeof(DijkstrasVTable));
      out->getNeighbors = (size_t(*)(Dijkstras*, QueueElem, QueueElem**))&_solverGetNeighbors;
      out->processNeighbor = (int(*)(Dijkstras*, QueueElem, QueueElem))&_solverProcessNeighbor;
      out->processCurrent = (int(*)(Dijkstras*, QueueElem, bool))&_solverProcessCurrent;
      out->destroy = (void(*)(Dijkstras*))&_solverDestroy;
   }
   return out;
}

size_t _solverGetNeighbors(GridSolver *self, GridNode *node, GridNode ***outList) {
   size_t i = node->index;

   int y = i / self->solveWidth;
   int x = i % self->solveWidth;
   int neighborCount = 0;
   *outList = node->neighbors;

   if (!(node->data.collision&GRID_SOLID_TOP) && y > 0) {
      GridNode *above = self->solvingTable->data + i - self->solveWidth;
      if (!(above->data.collision&GRID_SOLID_BOTTOM)) {
         node->neighbors[neighborCount++] = above;
      }
   }

   if (!(node->data.collision&GRID_SOLID_RIGHT) && x < self->solveWidth - 1) {
      GridNode *right = self->solvingTable->data + i + 1;
      if (!(right->data.collision&GRID_SOLID_LEFT)) {
         node->neighbors[neighborCount++] = right;
      }
   }

   if (!(node->data.collision&GRID_SOLID_BOTTOM) && y < self->solveHeight - 1) {
      GridNode *below = self->solvingTable->data + i + self->solveWidth;
      if (!(below->data.collision&GRID_SOLID_TOP)) {
         node->neighbors[neighborCount++] = below;
      }
   }

   if (!(node->data.collision&GRID_SOLID_LEFT) && x > 0) {
      GridNode *left = self->solvingTable->data + i - 1;
      if (!(left->data.collision&GRID_SOLID_RIGHT)) {
         node->neighbors[neighborCount++] = left;
      }
   }

   return neighborCount;
}
int _solverProcessNeighbor(GridSolver *self, GridNode *current, GridNode *node) {
   if (node && !node->visited) {
      float newScore = closureCall(&self->nFunc, &current->data, &node->data);
      if (newScore < node->score) {
         node->parent = current;
         node->score = newScore;
         return true;
      }
   }
   return false;
}
int _solverProcessCurrent(GridSolver *self, GridNode *node, bool last) {
   node->visited = true;
   GridNode *solution = (GridNode*)closureCall(&self->cFunc, &node->data, last);
   if (solution) {
      self->solutionNode = solution;
      return true;
   }

   return false;
}
void _solverDestroy(GridSolver *self) {
   priorityQueueDestroy(self->inner.queue);
   checkedFree(self);
}

static void _processSingleEntityCollision(GridSolver *self, Entity *e, Recti *r) {
   GridComponent *gc = entityGet(GridComponent)(e);
   int localX = gc->x - r->left;
   int localY = gc->y - r->top;
   if (localX >= 0 && localX < self->solveWidth &&
      localY >= 0 && localY < self->solveHeight) {

      GridNode *node = vecAt(GridNode)(self->solvingTable, localY * self->solveWidth + localX);
      node->data.e = e;
   }
}

static void _processEntityCollision(GridSolver *self, Recti *r) {
   //get entities in the area and mark solid collision
   gridManagerQueryEntitiesRect(self->manager, *r, self->eList);
   vecForEach(EntityPtr, e, self->eList, {
      _processSingleEntityCollision(self, *e, r);
   });
}

//we need to build our table to encompass our search area
static void _clearSolutionTable(GridSolver *self, size_t startCell) {
   int x = 0, y = 0;
   int ix, iy;
   size_t i = 0;
   Recti *r = &self->solutionSearchArea;

   gridManagerXYFromCellID(self->manager, startCell, &x, &y);

   r->left = MAX(0, x - DK_SEARCH_RADIUS);
   r->top = MAX(0, y - DK_SEARCH_RADIUS);
   r->right = MIN(gridManagerWidth(self->manager), x + DK_SEARCH_RADIUS + 1);
   r->bottom = MIN(gridManagerHeight(self->manager), y + DK_SEARCH_RADIUS + 1);

   self->solveWidth = rectiWidth(r);
   self->solveHeight = rectiHeight(r);
   self->solveCount = self->solveWidth * self->solveHeight;

   vecClear(GridNode)(self->solvingTable);
   vecResize(GridNode)(self->solvingTable, self->solveCount, &(GridNode){0});

   vecForEach(GridNode, node, self->solvingTable, {
      node->data.e = NULL;
   });

   _processEntityCollision(self, r);

   //clear the queue
   priorityQueueClear(self->inner.queue);

   for (iy = r->top; iy < r->bottom; ++iy) {
      for (ix = r->left; ix < r->right; ++ix) {
         GridNode *node = vecAt(GridNode)(self->solvingTable, i);
         size_t ID = gridManagerCellIDFromXY(self->manager, ix, iy);
         Tile *t = gridManagerTileAt(self->manager, ID);

         if (!t) {
            continue;
         }

         node->data.ID = ID;         
         node->index = i;

         node->data.collision |= t->collision;

         //target cell will be left as 0
         if (ix != x || iy != y) {
            node->score = INFF;
         }

         priorityQueuePush(self->inner.queue, node);
         ++i;
      }
   }
   
}

GridSolver *gridSolverCreate(WorldView *view) {
   GridSolver *out = checkedCalloc(1, sizeof(GridSolver));

   out->manager = view->managers->gridManager;
   out->solutionMap = vecCreate(GridSolutionNode)(NULL);
   out->solvingTable = vecCreate(GridNode)(NULL);
   out->eList = vecCreate(EntityPtr)(NULL);

   out->inner.vTable = _getSolverVTable();
   out->inner.queue = priorityQueueCreate(offsetof(GridNode, node), (PQCompareFunc)&_nodeCompareFunc);
   
   return out;
}
void gridSolverDestroy(GridSolver *self) {
   vecDestroy(GridNode)(self->solvingTable);
   vecDestroy(GridSolutionNode)(self->solutionMap);
   vecDestroy(EntityPtr)(self->eList);
   dijkstrasDestroy((Dijkstras*)self);
   //checkedFree(self);
}

GridSolution gridSolverSolve(GridSolver *self, size_t startCell, GridProcessCurrent cFunc, GridProcessNeighbor nFunc) {
   GridSolution solution = { INFF, INF, NULL };

   if (gridManagerTileAt(self->manager, startCell)) {
      GridNode *result = NULL;

      _clearSolutionTable(self, startCell);

      self->cFunc = cFunc;
      self->nFunc = nFunc;

      dijkstrasRun((Dijkstras*)self);

      result = self->solutionNode;

      if (result) {
         solution.solutionCell = result->data.ID;
         solution.totalCost = result->score;

         vecClear(GridSolutionNode)(self->solutionMap);

         while (result && result->parent) {
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