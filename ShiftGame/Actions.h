#pragma once

#include "Entities\Entities.h"
#include "segautils\Coroutine.h"
#include "segautils/Time.h"

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
   StringView ability;
}ActionAbilityNameComponent;
#define ComponentT ActionAbilityNameComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   float range;
}ActionRangeComponent;
#define ComponentT ActionRangeComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   StringView routine;
}ActionRoutineComponent;
#define ComponentT ActionRoutineComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   CombatAction *package;
}ActionDeliveryComponent;
#define ComponentT ActionDeliveryComponent
#include "Entities\ComponentDecl.h"

typedef Entity Status;
typedef struct {
   Status *parent;
}ActionGoverningStatusComponent;
#define ComponentT ActionGoverningStatusComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   EMPTY_STRUCT;
}ActionInvalidComponent;
#define ComponentT ActionInvalidComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   Seconds delay;
}ActionPreDelayComponent;
#define ComponentT ActionPreDelayComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   Action *sub;
}ActionSubActionComponent;
#define ComponentT ActionSubActionComponent
#include "Entities\ComponentDecl.h"

Action *actionCreateCustom(CommandManager *self);
Action *actionFromAbilityName(CommandManager *self, Entity *user, StringView name);
Action *actionFromAbilitySlot(CommandManager *self, Entity *user, size_t slot);

Action *actionTargetGridPosition(Action *self, int x, int y);
Action *actionTargetEntity(Action *self, Entity *target);
Action *actionSetRange(Action *self, float range);

//helpers
void actionHelperPushSlot(CommandManager *self, Entity *user, Entity *target, size_t slot);
void actionHelperPushSlotAtPosition(CommandManager *self, Entity *user, int x, int y, size_t slot);
void actionHelperPushAbility(CommandManager *self, Entity *user, Entity *target, StringView ability);
void actionHelperPushAbilityAtPosition(CommandManager *self, Entity *user, int x, int y, StringView ability);

void actionHelperPushFrontSlot(CommandManager *self, Entity *user, Entity *target, size_t slot);
void actionHelperPushFrontSlotAtPosition(CommandManager *self, Entity *user, int x, int y, size_t slot);
void actionHelperPushFrontAbility(CommandManager *self, Entity *user, Entity *target, StringView ability);
void actionHelperPushFrontAbilityAtPosition(CommandManager *self, Entity *user, int x, int y, StringView ability);

void actionHelperPushMoveToEntity(CommandManager *self, Entity *user, Entity *target, float range);
void actionHelperPushMoveToPosition(CommandManager *self, Entity *user, int x, int y, float range);
void actionHelperPushFrontMoveToEntity(CommandManager *self, Entity *user, Entity *target, float range);
void actionHelperPushFrontMoveToPosition(CommandManager *self, Entity *user, int x, int y, float range);

