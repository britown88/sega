#include "segautils\Preprocessor.h"
#include "segashared\RTTI.h"
#include "Entities.h"

#include "ComponentFunctions.h"

#define COMP_NAME CONCAT(comp_, ComponentT)

typedef struct {
   int parentID;
   ComponentT data;   
}COMP_NAME;

#define VectorT COMP_NAME
#include "segautils\Vector_Create.h"

#define T ComponentT

ImplRTTI(T);

ComponentListData *compListCreate(T)(){
   return (ComponentListData*)vecCreate(COMP_NAME)(NULL);
}

void compListDestroy(T)(ComponentListData data){
   vecDestroy(COMP_NAME)(data);
}

Component compListGetAt(T)(ComponentListData data, int index){
   if (index >= vecSize(COMP_NAME)(data)){
      return NULL;
   }

   return (Component)((byte *)vecAt(COMP_NAME)(data, index) + sizeof(int));
}

size_t compListCount(T)(ComponentListData data){
   return vecSize(COMP_NAME)(data);
}

int compListAddComp(T)(ComponentListData data, int entityID, Component comp){
   COMP_NAME c;
   memcpy(&c.data, comp, sizeof(T));
   c.parentID = entityID;
   vecPushBack(COMP_NAME)(data, &c);
   return vecSize(COMP_NAME)(data)-1;
}

void compListRemoveComp(T)(ComponentListData data, int index){
   memcpy(vecAt(COMP_NAME)(data, index), vecAt(COMP_NAME)(data, vecSize(COMP_NAME)(data)-1), sizeof(COMP_NAME));
   vecPopBack(COMP_NAME)(data);
}

ComponentVTable *compGetVTable(T)(){
   static ComponentVTable out = { 0 };
   static int init = 0;
   if (!init){
      out.create = (ComponentListData(*)())&compListCreate(T);
      out.destroy = (void(*)(ComponentListData))&compListDestroy(T);
      out.getAt = (Component(*)(ComponentListData, int))&compListGetAt(T);
      out.count = (size_t(*)(ComponentListData))&compListCount(T);
      out.add = (int(*)(ComponentListData, int, Component))&compListAddComp(T);
      out.remove = (void(*)(ComponentListData, int))&compListRemoveComp(T);

      init = 1;
   }

   return &out;
}

ComponentList *compList(T)(EntitySystem *system){
   size_t rtti = GetRTTI(T)()->ID;
   ComponentList *out = entitySystemGetCompList(system, rtti);
   if (!out){
      entitySystemRegisterCompList(system, rtti, compGetVTable(T)());
      out = entitySystemGetCompList(system, rtti);
   }
   return out;
}

T *entityGet(T)(Entity *self){
   EntitySystem *es = entityGetSystem(self);
   ComponentList *list = compList(T)(es);
   int ID = entityGetID(self);
   int compIndex = compListGetLookup(list)[ID];

   if (compIndex == -1){
      return NULL;
   }

   return compListGetVTable(list)->getAt(compListGetList(list), compIndex);

}
void entityAdd(T)(Entity *self, T *comp){
   EntitySystem *es = entityGetSystem(self);
   ComponentList *list = compList(T)(es);
   int ID = entityGetID(self);

   ComponentVTable *cvt = compListGetVTable(list);
   ComponentListData data = compListGetList(list);
   int *lookup = compListGetLookup(list);

   lookup[ID] = cvt->count(data);
   cvt->add(data, ID, comp);
}
void entityRemove(T)(Entity *self){
   EntitySystem *es = entityGetSystem(self);
   ComponentList *list = compList(T)(es);
   int ID = entityGetID(self);
   
   ComponentVTable *cvt = compListGetVTable(list);
   ComponentListData data = compListGetList(list);
   int *lookup = compListGetLookup(list);
   int currentIndex = lookup[ID];
   T *moved;

   lookup[ID] = -1;

   //removing swaps the end with the currentindex and pops back
   cvt->remove(data, currentIndex);

   //figure out which entity was moved and update its indices
   moved = cvt->getAt(data, currentIndex);
   if (moved){
      int movedEntity = componentGetParentID(moved);
      lookup[movedEntity] = currentIndex;
   }
}

#undef COMP_NAME
#undef ComponentT
#undef T