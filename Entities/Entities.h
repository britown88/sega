#pragma once

#include <stddef.h>
#include "segashared\RTTI.h"
#include <assert.h>

#define __FILE_EX__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

DeclLocalRTTITag(COMPONENTS)

#define MAX_ENTITIES (1024 * 128)

typedef struct EntitySystem_t EntitySystem;
typedef struct Entity_t Entity;
typedef Entity* EntityPtr;

#define VectorTPart EntityPtr
#include "segautils/Vector_Decl.h"

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

#define CreateManagerVTable(ManagerName) CONCAT(_createVTable_, ManagerName)()

#define ImplManagerVTable(ManagerName) \
   static void _destroy(ManagerName*); \
   static void _onDestroy(ManagerName*, Entity*); \
   static void _onUpdate(ManagerName*, Entity*); \
   static ManagerVTable *CreateManagerVTable(ManagerName){   \
      static ManagerVTable *out = NULL; \
      if (!out){ \
         out = calloc(1, sizeof(ManagerVTable)); \
         out->destroy = (void(*)(Manager*))&_destroy; \
         out->onDestroy = (void(*)(Manager*, Entity*))&_onDestroy; \
         out->onUpdate = (void(*)(Manager*, Entity*))&_onUpdate; \
      } \
      return out; \
   }

/*
Usage:

struct MyTestManager_t{
   Manager m;
   WorldView *view;
};

ImplManagerVTable(MyTestManager)

MyTestManager *createMyTestManager(WorldView *view){
   MyTestManager *out = checkedCalloc(1, sizeof(MyTestManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(MyTestManager);
   return out;
}

void _destroy(MyTestManager *self){
   checkedFree(self);
}
void _onDestroy(MyTestManager *self, Entity *e){}
void _onUpdate(MyTestManager *self, Entity *e){}

*/

EntitySystem *entitySystemCreateChecked(const char *file, int line);
EntitySystem *entitySystemCreateUnchecked();

#ifdef _DEBUG
#define entitySystemCreate() entitySystemCreateChecked(__FILE_EX__, __LINE__)
#else
#define entitySystemCreate() entitySystemCreateUnchecked()
#endif 


void entitySystemDestroy(EntitySystem *self);

void entitySystemRegisterManager(EntitySystem *self, Manager *manager);

size_t entitySystemGetManagerCount(EntitySystem *self);
Manager **entitySystemGetManagers(EntitySystem *self);

#define ClosureTPart \
    CLOSURE_RET(void) \
    CLOSURE_NAME(ComponentUpdate) \
    CLOSURE_ARGS(Entity*, Component)
#include "segautils\Closure_Decl.h"

void entitySystemUpdateComponent(EntitySystem *self, size_t compRtti, Entity *e, Component oldComponent);
void entitySystemRegisterComponentUpdate(EntitySystem *self, size_t compRtti, ComponentUpdate del);


Entity *entityCreateChecked(EntitySystem *system, const char *file, int line);
Entity *entityCreateUnchecked(EntitySystem *system);

#ifdef _DEBUG
#define entityCreate(SYSTEM) entityCreateChecked(SYSTEM, __FILE_EX__, __LINE__)
#else
#define entityCreate(SYSTEM) entityCreateUnchecked(SYSTEM)
#endif 

void entityUpdate(Entity *self);
void entityDestroy(Entity *self);
void entityVectorDestroy(EntityPtr *self);//for use with vector destructor
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

#define IF_COMPONENT(entity, component_type, varName, ...){\
   component_type *varName = entityGet(component_type)(entity); \
   if (varName){ \
      __VA_ARGS__ \
      } \
}

/*
IF_COMPONENT(e, PositionComponent, pc, {
pc->x = 5;
});
*/

#define COMPONENT_ADD(entity, component_type, ...) { \
   component_type CONCAT(__new_, component_type) = { __VA_ARGS__ }; \
   entityAdd(component_type)(entity, &CONCAT(__new_, component_type)); \
}

#define COMPONENT_QUERY(es, component_type, iterator_name, ...) { \
   size_t id = GetLocalRTTI(COMPONENTS, component_type)->ID; \
   ComponentList *CONCAT(clist__, component_type) = entitySystemGetCompList(es, id); \
   size_t CONCAT(count__, component_type); \
   compVerify(component_type)(es); \
   if(CONCAT(clist__, component_type) && (CONCAT(count__, component_type) = componentListGetCount(CONCAT(clist__, component_type)))) { \
      char* CONCAT(first__, component_type) = componentListGetRaw(CONCAT(clist__, component_type)); \
      char* CONCAT(last__, component_type) = CONCAT(first__, component_type) + CONCAT(count__, component_type) * (sizeof(int) + sizeof(component_type)); \
      while (CONCAT(first__, component_type) != CONCAT(last__, component_type)) {\
      component_type* iterator_name = (component_type*)(CONCAT(first__, component_type) += sizeof(int)); \
      __VA_ARGS__ \
      CONCAT(first__, component_type) += sizeof(component_type); \
      } \
   } \
}

#define COMPONENT_LOCK(Type, varName, entity, ...) {\
   Type* varName = entityGet(Type)(entity);\
   if (varName) {\
      Type CONCAT(OldComp__,Type) = *varName;\
      __VA_ARGS__\
      compBroadcastUpdate(Type)(entityGetSystem(entity), entity, &CONCAT(OldComp__,Type));\
   }\
}

void entitySystemRegisterCompList(EntitySystem *self, size_t rtti, ComponentVTable *table);
ComponentList *entitySystemGetCompList(EntitySystem *self, size_t rtti);





