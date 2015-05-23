#pragma once

#include "Entities\Entities.h"

typedef Entity Status;

typedef struct StatusManager_t StatusManager;

StatusManager *createStatusManager(WorldView *view);
void statusManagerUpdate(StatusManager *self);

Status *statusCreate(StatusManager *self);
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
   long duration, startTime;
}StatusDurationComponent;
#define ComponentT StatusDurationComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   EMPTY_STRUCT;
}StatusInflictsStunComponent;
#define ComponentT StatusInflictsStunComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   Action *child;
}StatusChildActionComponent;
#define ComponentT StatusChildActionComponent
#include "Entities\ComponentDecl.h"
