#pragma once

#include "Entities\Entities.h"
#include "segautils\Coroutine.h"

typedef struct CommandManager_t CommandManager;

typedef Entity Action;
typedef Action *ActionPtr;

typedef Entity CombatAction;

#define VectorTPart ActionPtr
#include "segautils\Vector_Decl.h"

typedef struct {
   Entity *user;
}ActionUserComponent;
#define ComponentT ActionUserComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   Entity *target;
}ActionTargetEntityComponent;
#define ComponentT ActionTargetEntityComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   int x, y;
}ActionTargetPositionComponent;
#define ComponentT ActionTargetPositionComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   float range;
}ActionRangeComponent;
#define ComponentT ActionRangeComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   size_t slot;
}ActionCombatComponent;
#define ComponentT ActionCombatComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   CombatAction *package;
}ActionDeliveryComponent;
#define ComponentT ActionDeliveryComponent
#include "Entities\ComponentDecl.h"

//put your action creating function decls here
Action *createActionGridPosition(CommandManager *self, int x, int y);
Action *createActionGridTarget(CommandManager *self, Entity *e, float range);
Action *createActionCombat(CommandManager *self, size_t slot, Entity *e);