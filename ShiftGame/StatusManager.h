#pragma once

#include "segautils/Time.h"
#include "Entities\Entities.h"
#include "WorldView.h"

typedef Entity Status;

#define ClosureTPart \
    CLOSURE_RET(Status *) \
    CLOSURE_NAME(StatusGenerator) \
    CLOSURE_ARGS(WorldView */*world*/)
#include "segautils\Closure_Decl.h"

typedef struct StatusManager_t StatusManager;
typedef struct StatusLibrary_t StatusLibrary;

StatusLibrary *statusLibraryCreate();
void statusLibraryDestroy(StatusLibrary *self);
void statusLibraryAdd(StatusLibrary *self, StringView name, StatusGenerator c);
StatusGenerator statusLibraryGet(StatusLibrary *self, StringView name);

StatusManager *createStatusManager(WorldView *view);
void statusManagerUpdate(StatusManager *self);

Status *statusCreateCustom(StatusManager *self);
Status *statusCreateByName(StatusManager *self, StringView name);
Status *statusSetDuration(Status *status, float duration);
Status *entityGetStatus(Entity *e, StringView name);
void entityAddStatus(StatusManager *self, Entity *e, Status *s);
void entityRemoveStatus(Entity *e, StringView name);

typedef struct{
   StringView name;
}StatusNameComponent;
#define ComponentT StatusNameComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   Entity *parent;
}StatusParentComponent;
#define ComponentT StatusParentComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   Seconds duration;
   Milliseconds startTime;
}StatusDurationComponent;
#define ComponentT StatusDurationComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   EMPTY_STRUCT;
}StatusInflictsStunComponent;
#define ComponentT StatusInflictsStunComponent
#include "Entities\ComponentDecl.h"

typedef Entity Action;
typedef struct{
   Action *child;
}StatusChildActionComponent;
#define ComponentT StatusChildActionComponent
#include "Entities\ComponentDecl.h"

void buildAllStatuses(StatusLibrary *self);
StatusGenerator buildStunStatus();