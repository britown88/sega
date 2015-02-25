#pragma once

#include <stddef.h>
#include "segashared\RTTI.h"

#define MAX_ENTITIES (1024 * 128)

typedef struct EntitySystem_t EntitySystem;
typedef struct Entity_t Entity;

typedef struct ComponentList_t ComponentList;
typedef void* ComponentListData;
typedef void* Component;

Entity *componentGetParent(Component self, EntitySystem *system);
int componentGetParentID(Component self);

typedef struct Manager_t Manager;

typedef struct {
   void(*onDestroy)(Manager*, Entity*);
   void(*onUpdate)(Manager*, Entity*);
   void(*destroy)(Manager*);
} ManagerVTable;

struct Manager_t {
   ManagerVTable *vTable;
};

void managerDestroy(Manager *self);
void managerOnDestroy(Manager *self, Entity *e);
void managerOnUpdate(Manager *self, Entity *e);

EntitySystem *entitySystemCreate();
void entitySystemDestroy(EntitySystem *self);

void entitySystemRegisterManager(EntitySystem *self, Manager *manager);
size_t entitySystemGetManagerCount(EntitySystem *self);
Manager **entitySystemGetManagers(EntitySystem *self);

Entity *entityCreate(EntitySystem *system);
void entityUpdate(Entity *self);
void entityDestroy(Entity *self);
int entityGetID(Entity *self);
EntitySystem *entityGetSystem(Entity *self);

typedef struct {
   ComponentListData(*create)();
   void(*destroy)(ComponentListData);
   Component(*getAt)(ComponentListData, int);
   size_t(*count)(ComponentListData);
   int(*add)(ComponentListData, int/*entity ID*/, Component);
   void(*remove)(ComponentListData, int/*entity ID*/);
   void*(*getRaw)(ComponentListData);
} ComponentVTable;

int *compListGetLookup(ComponentList *self);
ComponentListData compListGetList(ComponentList *self);
ComponentVTable *compListGetVTable(ComponentList *self);

void *componentListGetRaw(ComponentList *self);
size_t componentListGetCount(ComponentList *self);

#define COMPONENT_QUERY(es, component_type, iterator_name, ...) \
{ \
   size_t id = GetRTTI(component_type)()->ID; \
   ComponentList *CONCAT(clist__, component_type) = entitySystemGetCompList(es, id); \
   size_t CONCAT(count__, component_type); \
   if(CONCAT(clist__, component_type) && (CONCAT(count__, component_type) = componentListGetCount(CONCAT(clist__, component_type)))) \
   { \
      char* CONCAT(first__, component_type) = componentListGetRaw(CONCAT(clist__, component_type)); \
      char* CONCAT(last__, component_type) = CONCAT(first__, component_type) + CONCAT(count__, component_type) * (sizeof(int) + sizeof(component_type)); \
      while (CONCAT(first__, component_type) != CONCAT(last__, component_type))\
      {\
      component_type* iterator_name = (component_type*)(CONCAT(first__, component_type) += sizeof(int)); \
      __VA_ARGS__ \
      CONCAT(first__, component_type) += sizeof(component_type); \
      } \
   } \
}

void entitySystemRegisterCompList(EntitySystem *self, size_t rtti, ComponentVTable *table);
ComponentList *entitySystemGetCompList(EntitySystem *self, size_t rtti);





