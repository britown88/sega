#pragma once

#include <stddef.h>

#define MAX_ENTITIES (1024 * 128)

typedef struct EntitySystem_t EntitySystem;
typedef struct Entity_t Entity;

typedef struct ComponentList_t ComponentList;
typedef void* ComponentListData;
typedef void* Component;

Entity *componentGetParent(Component self, EntitySystem *system);
int componentGetParentID(Component self);

EntitySystem *entitySystemCreate();
void entitySystemDestroy(EntitySystem *self);

Entity *entityCreate(EntitySystem *system);
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
} ComponentVTable;

void entitySystemRegisterCompList(EntitySystem *self, size_t rtti, ComponentVTable *table);
ComponentList *entitySystemGetCompList(EntitySystem *self, size_t rtti);

int *compListGetLookup(ComponentList *self);
ComponentListData compListGetList(ComponentList *self);
ComponentVTable *compListGetVTable(ComponentList *self);

//void componentListAddComponent





