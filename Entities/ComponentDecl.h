#include "segautils\Preprocessor.h"
#include "segashared\RTTI.h"
#include "Entities.h"

#define T ComponentT

DeclRTTI(ComponentT);

#include "ComponentFunctions.h"

T *entityGet(T)(Entity *self);
void entityAdd(T)(Entity *self, T *comp);
void entityRemove(T)(Entity *self);

#undef ComponentT
#undef T