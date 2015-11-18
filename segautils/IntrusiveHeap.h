#pragma once

#include <stddef.h>
#include "Defs.h"

typedef void * QueueElem;
typedef struct QueueNode_t QueueNode;
typedef struct PriorityQueue_t PriorityQueue;

typedef QueueElem(*PQCompareFunc)(QueueElem, QueueElem);

struct QueueNode_t{
   QueueNode *next, *prev, *child;
};

void queueNodeClear(QueueNode *self);
void queueNodeUnlink(QueueNode *self);

PriorityQueue *priorityQueueCreate(size_t offset, PQCompareFunc min);
void priorityQueueDestroy(PriorityQueue *self);

void priorityQueueClear(PriorityQueue *self);

PriorityQueue *priorityQueueCreateUnchecked(size_t offset, PQCompareFunc min);
void priorityQueueDestroyUnchecked(PriorityQueue *self);

void priorityQueuePush(PriorityQueue *self, QueueElem data);
void priorityQueueReinsert(PriorityQueue *self, QueueElem data);
QueueElem priorityQueuePop(PriorityQueue *self);
int priorityQueueIsEmpty(PriorityQueue *self); 

void priorityQueueDecreaseKey(PriorityQueue *self, QueueElem data);

//********************
//dijkstra's
//********************
typedef struct Dijkstras_t Dijkstras;

typedef struct {
   size_t(*getNeighbors)(Dijkstras*, QueueElem, QueueElem**);
   int(*processNeighbor)(Dijkstras*, QueueElem, QueueElem);
   int(*processCurrent)(Dijkstras*, QueueElem, bool);
   void(*destroy)(Dijkstras*);
}DijkstrasVTable;

struct Dijkstras_t{
   DijkstrasVTable *vTable;
   PriorityQueue *queue;
};

QueueElem dijkstrasRun(Dijkstras *self);

size_t dijkstrasGetNeighbors(Dijkstras *self, QueueElem node, QueueElem **outList);
int dijkstrasProcessNeighbor(Dijkstras *self, QueueElem current, QueueElem neighbor);
int dijkstrasProcessCurrent(Dijkstras *self, QueueElem current, bool last);
void dijkstrasDestroy(Dijkstras *self);
