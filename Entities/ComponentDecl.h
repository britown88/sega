#include "segautils\Preprocessor.h"
#include "segashared\RTTI.h"
#include "Entities.h"

DeclRTTI(T);

#include "ComponentFunctions.h"

T *entityGet(T)(Entity *self);
void entityAdd(T)(Entity *self, T *comp);
void entityRemove(T)(Entity *self);

#undef T