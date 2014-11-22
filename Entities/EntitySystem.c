#include "Entities.h"
#include "segautils\IntrusiveHeap.h"
#include "segashared\CheckedMemory.h"
#include "segautils\Defs.h"

typedef struct Entity_t{ 
   QueueNode node;
   int ID;
   byte loaded;
   EntitySystem *system;
};

struct EntitySystem_t {
   Entity *entityPool;
   PriorityQueue *eQueue;
   size_t eCount;
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

Entity *entityCreate(EntitySystem *system){
   Entity *out;

   if (priorityQueueIsEmpty(system->eQueue)){
      int ID = system->eCount++;
      
      out = system->entityPool + ID;
      out->system = system;      
      out->ID = ID;
   }
   else {
      int ID = ((Entity*)priorityQueuePop(system->eQueue))->ID;
      out = system->entityPool + ID;
   }
   
   out->loaded = true;
   return out;
}

void entityDestroy(Entity *self){
   self->loaded = false;
   priorityQueuePush(self->system->eQueue, self);
}

int entityGetID(Entity *self){
   return self->ID;
}
EntitySystem *entityGetSystem(Entity *self){
   return self->system;
}