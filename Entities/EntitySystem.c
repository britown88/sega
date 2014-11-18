#include "Entities.h"
#include "segautils\IntrusiveHeap.h"
#include "segashared\CheckedMemory.h"
#include "segautils\Defs.h"

#define MAX_ENTITIES (1024 * 128)
typedef struct Entity_t{ 
   QueueNode node;
   int ID;
   unsigned char loaded;
   EntitySystem *system;
};

struct EntitySystem_t {
   Entity *entityPool;
   PriorityQueue *eQueue;
   int eCount;
};

Entity *_eNodeCompareFunc(Entity *n1, Entity *n2){
   return n1->ID < n2->ID ? n1 : n2;
}

EntitySystem *entitySystemCreate(){
   EntitySystem *out = checkedCalloc(1, sizeof(EntitySystem));
   out->eQueue = priorityQueueCreate(offsetof(Entity, node), (PQCompareFunc)&_eNodeCompareFunc);
   out->entityPool = checkedCalloc(MAX_ENTITIES, sizeof(Entity));

   return out;
}
void entitySystemDestroy(EntitySystem *self){   
   priorityQueueDestroy(self->eQueue);
   checkedFree(self->entityPool);
   checkedFree(self);
}

static Entity *_esLoadEntity(EntitySystem *self, int ID){
   Entity *out = self->entityPool + ID;
   out->system = self;
   out->ID = ID;
   out->loaded = true;

   return out;
}

Entity *entityCreate(EntitySystem *system){
   if (priorityQueueIsEmpty(system->eQueue)){
      return _esLoadEntity(system, system->eCount++);
   }
   else {
      Entity *popped = priorityQueuePop(system->eQueue);
      return _esLoadEntity(system, popped->ID);
   }
}

void entityDestroy(Entity *self){
   self->loaded = false;
   priorityQueuePush(self->system->eQueue, self);
}