#pragma once

#include "Managers.h"
#include "Entities\Entities.h"

typedef struct CombatManager_t CombatManager;

CombatManager *createCombatManager(WorldView *view);

typedef Entity CombatAction;

//create a new combat action
CombatAction *combatManagerCreateAction(CombatManager *self, Entity *source, Entity *target);

//declare your intent, returns the modified action to perform
CombatAction *combatManagerDeclareAction(CombatManager *self, CombatAction *proposed);

//query the final action at the time of infliction, returns the modified result to apply
CombatAction *combatManagerQueryActionResult(CombatManager *self, CombatAction *proposed);

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

