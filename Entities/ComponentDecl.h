#include "segautils\Preprocessor.h"
#include "segashared\RTTI.h"
#include "Entities.h"

#define T ComponentT

DeclLocalRTTI(COMPONENTS, ComponentT);
DeclRTTI(ComponentT);

#include "ComponentFunctions.h"

T *entityGet(T)(Entity *self);
void entityAdd(T)(Entity *self, T *comp);
void entityRemove(T)(Entity *self);

//allows the entity system to ensure proper component registration
//no use outside of entities.lib
void compVerify(T)(EntitySystem *system);

void compBroadcastUpdate(T)(EntitySystem *system, Entity *e, T *oldComponent);
void compRegisterUpdateDelegate(T)(EntitySystem *system, ComponentUpdate del);

#undef ComponentT
#undef T