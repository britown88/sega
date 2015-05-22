#pragma once

#include "segashared\Strings.h"

#include "WorldView.h"
#include "Actions.h"


#define ClosureTPart \
    CLOSURE_RET(Action *) \
    CLOSURE_NAME(AbilityGenerator) \
    CLOSURE_ARGS(WorldView */*world*/, Entity */*user*/)
#include "segautils\Closure_Decl.h"

typedef struct AbilityLibrary_t AbilityLibrary;

AbilityLibrary *abilityLibraryCreate();
void abilityLibraryDestroy(AbilityLibrary *self);

void abilityLibraryAdd(AbilityLibrary *self, StringView name, AbilityGenerator c);
AbilityGenerator abilityLibraryGet(AbilityLibrary *self, StringView name);

void buildAllAbilities(AbilityLibrary *self);

AbilityGenerator buildAutoAttackAbility();
AbilityGenerator buildMagicMissileAbility();
AbilityGenerator buildMeleeAttackAbility();
AbilityGenerator buildMoveAbility();
AbilityGenerator buildRangedAttackAbility();
AbilityGenerator buildSwapAbility();

