#pragma once

#include <stddef.h>

typedef struct ComponentSystem_t ComponentSystem;
typedef struct ComponentList_t ComponentList;
typedef void* ComponentListData;
typedef void* Component;

typedef struct {
   ComponentListData(*create)();
   void(*destroy)(ComponentListData);
   Component(*getAt)(ComponentListData, int);
   int(*count)(ComponentListData);
   int(*add)(ComponentListData, int, Component);
} ComponentVTable;

ComponentSystem *componentSystemCreate();
void componentSystemDestroy(ComponentSystem *self);
void componentSystemRegisterList(ComponentSystem *self, size_t rtti, ComponentVTable *table);
ComponentList *componentSystemGetList(ComponentSystem *self, size_t rtti);