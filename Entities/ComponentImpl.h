#include "segautils\Preprocessor.h"
#include "segashared\RTTI.h"
#include "Entities.h"

#include "ComponentFunctions.h"

#define COMP_NAME CONCAT(comp_, ComponentT)

#pragma pack(push, 1)
typedef struct {
   int parentID;
   ComponentT data;   
}COMP_NAME;
#pragma pack(pop)

#define VectorT COMP_NAME
#include "segautils\Vector_Create.h"

#define T ComponentT

ImplLocalRTTI(COMPONENTS, T);
ImplRTTI(T);

ComponentListData *compListCreate(T)(){
   return (ComponentListData*)vecCreate(COMP_NAME)(NULL);
}

void compListDestroy(T)(ComponentListData data){
   vecDestroy(COMP_NAME)(data);
}

Component compListGetAt(T)(ComponentListData data, size_t index){
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
   int swapIndex = vecSize(COMP_NAME)(data) - 1;
   COMP_NAME *toDelete = vecAt(COMP_NAME)(data, index);
   COMP_NAME *last = vecAt(COMP_NAME)(data, swapIndex);

#ifdef COMP_DESTROY_FUNC
   COMP_DESTROY_FUNC(&toDelete->data);
   #undef COMP_DESTROY_FUNC
#endif

   if (swapIndex != index){
      memcpy(toDelete, last, sizeof(COMP_NAME));
   }
   vecPopBack(COMP_NAME)(data);
}

void *compListGetRaw(T)(ComponentListData data){
   return ((vec(COMP_NAME)*)data)->data;
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
      out.getRaw = (void*(*)(ComponentListData))&compListGetRaw(T);

      init = 1;
   }

   return &out;
}

ComponentList *compList(T)(EntitySystem *system){
   size_t rtti = GetLocalRTTI(COMPONENTS, T)->ID;
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
   ComponentVTable *cvt = compListGetVTable(list);
   ComponentListData data = compListGetList(list);

   if (compIndex == -1){
      return NULL;
   }

   return cvt->getAt(data, compIndex);

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

void compBroadcastUpdate(T)(EntitySystem *system, Entity *e, T *oldComponent){
   entitySystemUpdateComponent(system, GetLocalRTTI(COMPONENTS, T)->ID, e, oldComponent);
}
void compRegisterUpdateDelegate(T)(EntitySystem *system, ComponentUpdate del){
   compList(T)(system);//ensure component gets registered for RTTI to line up correctly
   entitySystemRegisterComponentUpdate(system, GetLocalRTTI(COMPONENTS, T)->ID, del);
}
void compVerify(T)(EntitySystem *system){
   compList(T)(system);
}

#undef COMP_NAME
#undef ComponentT
#undef T