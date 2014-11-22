#include "Components.h"
#include "Entities.h"

#include "segashared\CheckedMemory.h"
#include "segautils\BitTwiddling.h"

struct  ComponentList_t{
   ComponentVTable *cvt;

   ComponentListData list;
   int lookup[MAX_ENTITIES];
};

#define T ComponentList
#include "segautils\Vector_Create.h"

struct ComponentSystem_t {
   vec(ComponentList) *lists;
};

void _cvtDestroy(ComponentList *self){
   if (self->cvt->destroy){
      self->cvt->destroy(self->list);
   }   
}

ComponentSystem *componentSystemCreate(){
   ComponentSystem *out = checkedCalloc(1, sizeof(ComponentSystem));
   out->lists = vecCreate(ComponentList)(&_cvtDestroy);
   return out;
}
void componentSystemDestroy(ComponentSystem *self){
   vecDestroy(ComponentList)(self->lists);
   checkedFree(self);
}
void componentSystemRegisterList(ComponentSystem *self, size_t rtti, ComponentVTable *table){
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
ComponentList *componentSystemGetList(ComponentSystem *self, size_t rtti){
   return vecAt(ComponentList)(self->lists, rtti);
}