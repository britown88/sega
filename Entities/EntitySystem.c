#include "Entities.h"
#include "segautils\IntrusiveHeap.h"
#include "segashared\CheckedMemory.h"
#include "segautils\Defs.h"
#include "segautils\BitTwiddling.h"

ImplLocalRTTITag(COMPONENTS)

#define ClosureTPart CLOSURE_NAME(ComponentUpdate)
#include "segautils\Closure_Impl.h"

#define VectorT ComponentUpdate
#include "segautils/Vector_Create.h"

typedef struct {
   size_t key;
   vec(ComponentUpdate) *value;
} cudEntry;

#define HashTableT cudEntry
#include "segautils\HashTable_Create.h"

static int _cudEntryCompare(cudEntry *e1, cudEntry *e2){
   return e1->key == e2->key;
}

static size_t _cudEntryHash(cudEntry *p){
   return p->key;
}

static void _cudEntryDestroy(cudEntry *p){
   vecDestroy(ComponentUpdate)(p->value);
}

static void _compUpdateDestroy(ComponentUpdate *self){
   closureDestroy(ComponentUpdate)(self);
}

typedef struct Entity_t{ 
   QueueNode node;
   int ID;
   byte loaded;
   EntitySystem *system;
};

#define VectorTPart EntityPtr
#include "segautils/Vector_Impl.h"

struct ComponentList_t{
   ComponentVTable *cvt;

   ComponentListData list;
   int lookup[MAX_ENTITIES];
};

int *compListGetLookup(ComponentList *self){
   return self->lookup;
}

ComponentListData compListGetList(ComponentList *self){
   return self->list;
}

ComponentVTable *compListGetVTable(ComponentList *self){
   return self->cvt;
}

void *componentListGetRaw(ComponentList *self){
   return self->cvt->getRaw(self->list);
}
size_t componentListGetCount(ComponentList *self){
   return self->cvt->count(self->list);
}

#define VectorT ComponentList
#include "segautils\Vector_Create.h"

typedef Manager* ManagerPtr;
#define VectorT ManagerPtr
#include "segautils\Vector_Create.h"

void _cvtDestroy(ComponentList *self){
   if (self->cvt && self->cvt->destroy){
      self->cvt->destroy(self->list);
   }
}

struct EntitySystem_t {
   Entity *entityPool;
   PriorityQueue *eQueue;
   size_t eCount;

   vec(ComponentList) *lists;
   vec(ManagerPtr) *managers;
   ht(cudEntry) *updateDelegates;
};

Entity *_eNodeCompareFunc(Entity *n1, Entity *n2){
   return n1->ID < n2->ID ? n1 : n2;
}

void managerDestroy(Manager *self){
   self->vTable->destroy(self);
}
static void _managerVDestroy(ManagerPtr *self){
   managerDestroy(*self);
}
void managerOnDestroy(Manager *self, Entity *e){
   self->vTable->onDestroy(self, e);
}
void managerOnUpdate(Manager *self, Entity *e){
   self->vTable->onUpdate(self, e);
}

EntitySystem *entitySystemCreate(){
   EntitySystem *out = checkedCalloc(1, sizeof(EntitySystem));
   out->eQueue = priorityQueueCreate(offsetof(Entity, node), (PQCompareFunc)&_eNodeCompareFunc);
   out->entityPool = checkedCalloc(MAX_ENTITIES, sizeof(Entity));
   out->lists = vecCreate(ComponentList)(&_cvtDestroy);
   out->managers = vecCreate(ManagerPtr)(&_managerVDestroy);
   out->updateDelegates = htCreate(cudEntry)(&_cudEntryCompare, &_cudEntryHash, &_cudEntryDestroy);

   return out;
}

void _destroyAllEntities(EntitySystem *self){
   Entity *first = self->entityPool;
   Entity *last = first + self->eCount;
   while (first != last){
      Entity *e = first++;
      if (e->loaded){
         entityDestroy(e);
      }
   }

}

void entitySystemDestroy(EntitySystem *self){   
   _destroyAllEntities(self);
   priorityQueueDestroy(self->eQueue);
   checkedFree(self->entityPool);
   vecDestroy(ComponentList)(self->lists);
   vecDestroy(ManagerPtr)(self->managers);
   htDestroy(cudEntry)(self->updateDelegates);
   checkedFree(self);
}
void entitySystemRegisterManager(EntitySystem *self, Manager *manager){
   vecPushBack(ManagerPtr)(self->managers, &manager);
}
size_t entitySystemGetManagerCount(EntitySystem *self){
   return vecSize(ManagerPtr)(self->managers);
}
Manager **entitySystemGetManagers(EntitySystem *self){
   if (vecIsEmpty(ManagerPtr)(self->managers)){
      return NULL;
   }
   else {
      return self->managers->data;
   }
}

void entitySystemUpdateComponent(EntitySystem *self, size_t compRtti, Entity *e, Component oldComponent){
   cudEntry entry = { compRtti, NULL };
   cudEntry *found = htFind(cudEntry)(self->updateDelegates, &entry);
   if (found){
      vecForEach(ComponentUpdate, cud, found->value, {
         closureCall(cud, e, oldComponent);
      });
   }
}
void entitySystemRegisterComponentUpdate(EntitySystem *self, size_t compRtti, ComponentUpdate del){
   cudEntry entry = { compRtti, NULL };
   cudEntry *found = htFind(cudEntry)(self->updateDelegates, &entry);
   vec(ComponentUpdate) *delegateList = NULL;
   if (!found){
      //add a new list
      delegateList = vecCreate(ComponentUpdate)(&_compUpdateDestroy);
      entry.value = delegateList;
      htInsert(cudEntry)(self->updateDelegates, &entry);
   }
   else{
      delegateList = found->value;
   }

   vecPushBack(ComponentUpdate)(delegateList, &del);
}

void entitySystemRegisterCompList(EntitySystem *self, size_t rtti, ComponentVTable *table){
   ComponentList cl = { 0 };
   int undef = -1;
   size_t listCount = vecSize(ComponentList)(self->lists);

   cl.cvt = table;
   if (table->create){
      cl.list = table->create();
   }

   //init to -1
   STOSD((unsigned long*)cl.lookup, *(unsigned long*)&undef, MAX_ENTITIES);

   if (rtti >= listCount){
      vecResize(ComponentList)(self->lists, rtti + 1, NULL);
   }

   //copy component list into the lists vector
   memcpy(vecAt(ComponentList)(self->lists, rtti), &cl, sizeof(cl));
}
ComponentList *entitySystemGetCompList(EntitySystem *self, size_t rtti){
   ComponentList *out = NULL;
   if (rtti >= vecSize(ComponentList)(self->lists)){
      return NULL;
   }

   out = vecAt(ComponentList)(self->lists, rtti);

   return out;
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

void entityUpdate(Entity *self){
   Manager **first = entitySystemGetManagers(self->system);
   Manager **last = first + entitySystemGetManagerCount(self->system);

   while (first != last){ managerOnUpdate((*first++), self); }
}

void _callManagerDestroy(Entity *self){
   Manager **first = entitySystemGetManagers(self->system);
   Manager **last = first + entitySystemGetManagerCount(self->system);

   while (first != last){ managerOnDestroy((*first++), self); }
}

void entityVectorDestroy(EntityPtr *self){
   entityDestroy(*self);  
}

static void _destroyComponent(Entity *self, ComponentList *list){
   int compIndex = list->lookup[self->ID];
   Component moved;

   if (compIndex == -1){
      //component doesnt fucking exist so dont fucking delete it nerd
      return;
   }

   list->lookup[self->ID] = -1;

   list->cvt->remove(list->list, compIndex);
   moved = list->cvt->getAt(list->list, compIndex);
   if (moved){
      int movedEntity = componentGetParentID(moved);
      list->lookup[movedEntity] = compIndex;
   }
}

void entityDestroy(Entity *self){
   vec(ComponentList) *v = self->system->lists;

   _callManagerDestroy(self);

   vecForEach(ComponentList, list, v, {
      if (list->list){
         _destroyComponent(self, list);
      }
   });

   self->loaded = false;
   priorityQueuePush(self->system->eQueue, self);
}

Entity *componentGetParent(Component self, EntitySystem *system){
   return system->entityPool + componentGetParentID(self);
}
int componentGetParentID(Component self){
   return *(int*)((byte *)self - sizeof(int));
}

int entityGetID(Entity *self){
   return self->ID;
}
EntitySystem *entityGetSystem(Entity *self){
   return self->system;
}