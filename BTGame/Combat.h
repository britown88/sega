#pragma once

#include "Managers.h"
#include "Entities\Entities.h"

typedef enum{
   Fire = 0,
   Earth,
   Wind,
   Water
} Element;

typedef struct{
   float HP, strength, agility, intelligence;
   float affinity[4];
}StatsComponent;
#define ComponentT StatsComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   float HP, strength, agility, intelligence, armor;
   float affinity[4];
}StatModsComponent;
#define ComponentT StatModsComponent
#include "Entities\ComponentDecl.h"

bool entityIsDead(Entity *e);
float entityGetMaxHP(Entity *e);
float entityGetStrength(Entity *e);
float entityGetAgility(Entity *e);
float entityGetIntelligence(Entity *e);
float entityGetArmor(Entity *e);

typedef struct CombatManager_t CombatManager;

CombatManager *createCombatManager(WorldView *view);
void combatManagerUpdate(CombatManager *self);

typedef Entity CombatAction;

//create a new combat action
CombatAction *combatManagerCreateAction(CombatManager *self, Entity *source, Entity *target);

//declare your intent, returns the modified action to perform
CombatAction *combatManagerDeclareAction(CombatManager *self, CombatAction *proposed);

//query the final action at the time of infliction, returns the modified result to apply
CombatAction *combatManagerQueryActionResult(CombatManager *self, CombatAction *proposed);

//Apply the stat and state changes of an action directly
void combatManagerExecuteAction(CombatManager *self, CombatAction *action);

typedef struct{
   Entity *source;
}CActionSourceComponent;
#define ComponentT CActionSourceComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   Entity *target;
}CActionTargetComponent;
#define ComponentT CActionTargetComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   EMPTY_STRUCT;
}CActionCancelledComponent;
#define ComponentT CActionCancelledComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   float range;
}CActionRangeComponent;
#define ComponentT CActionRangeComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   float damage;
}CActionDamageComponent;
#define ComponentT CActionDamageComponent
#include "Entities\ComponentDecl.h"

typedef enum{
   DamageTypePhysical,
   DamageTypeMagical,
   DamageTypeChaos
}DamageType;

typedef struct{
   DamageType type;
}CActionDamageTypeComponent;
#define ComponentT CActionDamageTypeComponent
#include "Entities\ComponentDecl.h"

