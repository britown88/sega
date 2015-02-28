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
};


Node *_nodeCompareFunc(Node *n1, Node *n2){
   return n1->score < n2->score ? n1 : n2;
}

void buildTable(Node *nodes){
   int i;
   for (i = 0; i < CELL_COUNT; ++i){
      nodes[i].ID = i;
      nodes[i].score = INF;
   }
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

void checkNeighbor(PriorityQueue *pq, Node *table, Node *current, size_t newScore, size_t index){
   if (index >= 0 || index < CELL_COUNT){
      Node *neighbor = table + index;
      
      if (!neighbor->visited){
         if (newScore < neighbor->score){
            neighbor->parent = current;
            neighbor->score = newScore;
            priorityQueueDecreaseKey(pq, neighbor);
         }
      }
   }
}

void derpjkstras(int srcX, int srcY, int destX, int destY){
   int i;
   size_t destination = indexFromXY(destX, destY);
   size_t source = indexFromXY(srcX, srcY);
   Node *current;
   Node table[CELL_COUNT] = { 0 };
   PriorityQueue *pq = priorityQueueCreate(offsetof(Node, node), (PQCompareFunc)&_nodeCompareFunc);
   buildTable(table);

   table[source].score = 0;

   for (i = 0; i < CELL_COUNT; ++i){
      priorityQueuePush(pq, table + i);
   }


   while (!priorityQueueIsEmpty(pq)){
      size_t up, right, left, down;
      int x, y;
      size_t nextScore;
      current = priorityQueuePop(pq);
      nextScore = current->score + 1;

      if (current->ID == destination){
         break;
      }
      
      XYFromIndex(current->ID, &x, &y);
      checkNeighbor(pq, table, current, nextScore, indexFromXY(x, y - 1));
      checkNeighbor(pq, table, current, nextScore, indexFromXY(x, y + 1));
      checkNeighbor(pq, table, current, nextScore, indexFromXY(x - 1, y));
      checkNeighbor(pq, table, current, nextScore, indexFromXY(x + 1, y));
      
      
      current->visited = true;

   }


   priorityQueueDestroy(pq);
}