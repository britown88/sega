#include "IntrusiveHeap.h"
#include "segashared\CheckedMemory.h"
#include "segautils\Defs.h"

QueueElem dijkstrasRun(Dijkstras *self){
   while (!priorityQueueIsEmpty(self->queue)){
      size_t nCount = 0, nodeSize = 0;
      QueueElem *nBegin = NULL, *nEnd = NULL;
      QueueElem current = priorityQueuePop(self->queue);      

      if (dijkstrasProcessCurrent(self, current, priorityQueueIsEmpty(self->queue))){
         return current;
      }

      nCount = dijkstrasGetNeighbors(self, current, &nBegin);
      if (nCount){
         nEnd = nBegin + nCount;
         while (nBegin != nEnd){
            QueueElem n = *nBegin++;
            if (dijkstrasProcessNeighbor(self, current, n)){
               priorityQueueDecreaseKey(self->queue, n);
            }
         }
      }

      
   }

   return NULL;
}

size_t dijkstrasGetNeighbors(Dijkstras *self, QueueElem node, QueueElem **outList){
   return self->vTable->getNeighbors(self, node, outList);
}
int dijkstrasProcessNeighbor(Dijkstras *self, QueueElem current, QueueElem node){
   return self->vTable->processNeighbor(self, current, node);
}
int dijkstrasProcessCurrent(Dijkstras *self, QueueElem node, bool last){
   return self->vTable->processCurrent(self, node, last);
}
void dijkstrasDestroy(Dijkstras *self){
   self->vTable->destroy(self);
}



