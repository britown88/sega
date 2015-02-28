#include "segautils\IntrusiveHeap.h"
#include "segashared\CheckedMemory.h"
#include "segautils\Defs.h"
#include "segautils\BitTwiddling.h"

#define TABLE_WIDTH 12
#define TABLE_HEIGHT 8
#define CELL_COUNT (TABLE_WIDTH*TABLE_HEIGHT)
#define INF ((size_t)-1)

typedef struct Node_t Node;

struct Node_t{
   QueueNode node;
   byte visited;
   size_t score;
   int ID;
   Node *parent;
   Node *neighbors[4];
};


Node *_nodeCompareFunc(Node *n1, Node *n2){
   return n1->score < n2->score ? n1 : n2;
}

typedef struct {
   Dijkstras inner;
   size_t destination;
}GridDK;

static size_t _getNeighbors(GridDK *self, Node *node, Node ***outList);
static int _processNeighbor(GridDK *self, Node *current, Node *node);
static int _processCurrent(GridDK *self, Node *node);
static void _destroy(GridDK *self);

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

size_t _getNeighbors(GridDK *self, Node *node, Node ***outList){
   *outList = node->neighbors;
   return 4;
}
int _processNeighbor(GridDK *self, Node *current, Node *node){
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
int _processCurrent(GridDK *self, Node *node){
   node->visited = true;

   return node->score == INF || node->ID == self->destination;
}
void _destroy(GridDK *self){
   priorityQueueDestroy(self->inner.queue);
   checkedFree(self);
}

size_t indexFromXY(int x, int y){
   if (x < 0 || x >= TABLE_WIDTH || y < 0 || y >= TABLE_HEIGHT){
      return INF;
   }

   return TABLE_WIDTH * y + x;
}

void XYFromIndex(size_t index, int*x, int*y){
   *x = index % TABLE_WIDTH;
   *y = index / TABLE_WIDTH;
}

void addNeighbors(Node *nodes, size_t current){
   int x, y, i;
   size_t nID[4];
   Node *node = nodes + current;
   XYFromIndex(node->ID, &x, &y);

   nID[0] = indexFromXY(x, y-1);
   nID[1] = indexFromXY(x-1, y);
   nID[2] = indexFromXY(x+1, y);
   nID[3] = indexFromXY(x, y+1);

   for (i = 0; i < 4; ++i){
      if (nID[i] < INF){
         node->neighbors[i] = nodes + nID[i];
      }
   }
}

void buildTable(Node *nodes){
   int i;
   for (i = 0; i < CELL_COUNT; ++i){
      nodes[i].ID = i;
      nodes[i].score = INF;
      addNeighbors(nodes , i);
   }
}

typedef struct{
   int x, y;
}GridPos;

#define VectorT GridPos
#include "segautils\Vector_Create.h"


void derpjkstras(int srcX, int srcY, int destX, int destY){
   int i;
   Node table[CELL_COUNT] = { 0 };//our grid
   PriorityQueue *pq = priorityQueueCreate(offsetof(Node, node), (PQCompareFunc)&_nodeCompareFunc);
   GridDK *dk = checkedCalloc(1, sizeof(GridDK));
   Node *result = NULL;
   vec(GridPos) *path = vecCreate(GridPos)(NULL);

   buildTable(table);
   table[indexFromXY(srcX, srcY)].score = 0;

   for (i = 0; i < CELL_COUNT; ++i){
      priorityQueuePush(pq, table + i);
   }

   dk->inner.vTable = _getVTable();
   dk->inner.queue = pq;
   dk->destination = indexFromXY(destX, destY);

   result = dijkstrasRun((Dijkstras*)dk);

   while (result){
      GridPos pos;
      XYFromIndex(result->ID, &pos.x, &pos.y);
      vecPushBack(GridPos)(path, &pos);

      result = result->parent;
   }

   vecReverse(GridPos)(path);

   dijkstrasDestroy((Dijkstras*)dk);
   vecDestroy(GridPos)(path);

}
