#include "IntrusiveHeap.h"
#include "segashared\CheckedMemory.h"

#include <stddef.h>

struct PriorityQueue_t{
   QueueNode *head;
   size_t offset;

   PQCompareFunc min;
};

PriorityQueue *priorityQueueCreate(size_t offset, PQCompareFunc min){
   PriorityQueue *r = checkedCalloc(1, sizeof(PriorityQueue));
   r->min = min;
   r->offset = offset;

   return r;   
}
void priorityQueueDestroy(PriorityQueue *self){
   checkedFree(self);
}

void priorityQueueClear(PriorityQueue *self){
   self->head = NULL;
}

PriorityQueue *priorityQueueCreateUnchecked(size_t offset, PQCompareFunc min){
   PriorityQueue *r = calloc(1, sizeof(PriorityQueue));

   r->min = min;
   r->offset = offset;

   return r;
}
void priorityQueueDestroyUnchecked(PriorityQueue *self){
   free(self);
}

static QueueNode *priorityQueueGetNode(PriorityQueue *self, QueueElem data){
   return (QueueNode *)((char *)data + self->offset);
}

static QueueElem priorityQueueGetElem(PriorityQueue *self, QueueNode *node){
   return (QueueElem)((char *)node - self->offset);
}

static void push_front(QueueNode *parent, QueueNode *child){
   if (parent->child){
      parent->child->prev = child;
   }

   queueNodeUnlink(child);

   child->prev = parent;
   child->next = parent->child;
   parent->child = child;
}

static QueueNode *priorityQueueMerge(PriorityQueue *self, QueueNode *h1, QueueNode *h2){
   QueueNode *root, *other;

   if (!h1){ return h2; }
   if (!h2){ return h1; }

   root = priorityQueueGetNode(self, self->min(priorityQueueGetElem(self, h1), priorityQueueGetElem(self, h2)));//O FUCKING K
   other = root == h1 ? h2 : h1;

   push_front(root, other);

   return root;
}

static QueueNode *priorityQueueMergePairs(PriorityQueue *self, QueueNode *node){
   QueueNode *out, *prev;

   if (!node){
      return NULL;
   }

   out = node;
   do
   {
      out = node = priorityQueueMerge(self, node, node->next);
      node = node->next;  //skip a node, merge in pairs.
   } while (node);

   prev = out->prev;
   while (prev->child != out){
      out = priorityQueueMerge(self, out, prev);
      prev = out->prev;
   }

   return out;

}

void priorityQueueReinsert(PriorityQueue *self, QueueElem data){
   QueueNode *newNode = priorityQueueGetNode(self, data);

   self->head = priorityQueueMerge(self, self->head, newNode);
}

void priorityQueuePush(PriorityQueue *self, QueueElem data){
   QueueNode *newNode = priorityQueueGetNode(self, data);
   newNode->next = newNode->prev = newNode->child = NULL;

   self->head = priorityQueueMerge(self, self->head, newNode);
}

QueueElem priorityQueuePop(PriorityQueue *self){
   QueueElem out;

   if (!self->head){
      return NULL;
   }

   out = priorityQueueGetElem(self, self->head);
   self->head = priorityQueueMergePairs(self, self->head->child);

   return out;
}

int priorityQueueIsEmpty(PriorityQueue *self){
   return !self->head;
}

void queueNodeClear(QueueNode *self){
   self->child = self->next = self->prev = NULL;
}

void queueNodeUnlink(QueueNode *self){
   if (self->prev){
      if (self->prev->child == self){
         //prev is my parent
         self->prev->child = self->next;
      }
      else{
         self->prev->next = self->next;
      }
   }

   if (self->next){
      self->next->prev = self->prev;
   }
}

void priorityQueueDecreaseKey(PriorityQueue *self, QueueElem data){
   QueueNode *node = priorityQueueGetNode(self, data);

   //already smallest
   if (node == self->head){
      return;
   }

   queueNodeUnlink(node);
   node->next = node->prev = NULL;

   priorityQueueReinsert(self, data);
}