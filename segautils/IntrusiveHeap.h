#pragma once

#include <stddef.h>

typedef void * QueueElem;
typedef struct QueueNode_t QueueNode;
typedef struct PriorityQueue_t PriorityQueue;

typedef QueueElem(*PQCompareFunc)(QueueElem, QueueElem);

struct QueueNode_t{
   QueueNode *next, *prev, *child;
};

void queueNodeUnlink(QueueNode *self);

PriorityQueue *priorityQueueCreate(size_t offset, PQCompareFunc min);
void priorityQueueDestroy(PriorityQueue *self);

void priorityQueuePush(PriorityQueue *self, QueueElem data);
QueueElem priorityQueuePop(PriorityQueue *self);
int priorityQueueIsEmpty(PriorityQueue *self); 

void priorityQueueDecreaseKey(PriorityQueue *self, QueueElem data);