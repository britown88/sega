#include "Entities.h"
#include "segautils\IntrusiveHeap.h"
#include "segashared\CheckedMemory.h"
#include "segautils\Defs.h"

#define MAX_ENTITIES (1024 * 128)
typedef struct { 
   QueueNode node;
   int ID;
   unsigned char loaded;
}EntityNode;

struct EntitySystem_t {
   EntityNode *nodePool;
   PriorityQueue *eQueue;
   int eCount;
};

EntityNode *_eNodeCompareFunc(EntityNode *n1, EntityNode *n2){
   return n1->ID < n2->ID ? n1 : n2;
}

EntitySystem *entitySystemCreate(){
   EntitySystem *out = checkedCalloc(1, sizeof(EntitySystem));
   out->eQueue = priorityQueueCreate(offsetof(EntityNode, node), (PQCompareFunc)&_eNodeCompareFunc);
   out->nodePool = checkedCalloc(MAX_ENTITIES, sizeof(EntityNode));

   return out;
}
void entitySystemDestroy(EntitySystem *self){   
   priorityQueueDestroy(self->eQueue);
   checkedFree(self->nodePool);
   checkedFree(self);
}

static Entity _esLoadEntity(EntitySystem *self, int ID){
   Entity out = { ID, self };
   (self->nodePool + ID)->loaded = true;

   return out;
}

Entity entitySystemCreateEntity(EntitySystem *self){
   if (priorityQueueIsEmpty(self->eQueue)){
      return _esLoadEntity(self, self->eCount++);
   }
   else {
      EntityNode *popped = priorityQueuePop(self->eQueue);
      return _esLoadEntity(self, popped->ID);
   }
}
void entitySystemDestroyEntity(EntitySystem *self, int entityID){
   EntityNode *node = self->nodePool + entityID;
   node->loaded = false;
   node->ID = entityID;

   priorityQueuePush(self->eQueue, node);
}