#include "segautils\Preprocessor.h"
#include "segashared\RTTI.h"
#include "Entities.h"

ImplRTTI(T);

#include "ComponentFunctions.h"

#define SKIP_T_UNDEF 1
#include "segautils\Vector_Create.h"
#undef SKIP_T_UNDEF

ComponentListData *compListCreate(T)(){
   return (ComponentListData*)vecCreate(T)(NULL);
}

void compListDestroy(T)(ComponentListData data){
   vecDestroy(T)(data);
}

Component compListGetAt(T)(ComponentListData data, int index){
   return (Component)vecAt(T)(data, index);
}

size_t compListCount(T)(ComponentListData data){
   return vecSize(T)(data);
}

int compListAddComp(T)(ComponentListData data, int entityID, Component comp){
   vecPushBack(T)(data, comp);
   return vecSize(T)(data)-1;
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

}

#undef T