#pragma once

#include "segautils\Coroutine.h"
#include "segashared\Strings.h"

#include "WorldView.h"
#include "Actions.h"

#define ClosureTPart \
    CLOSURE_RET(Coroutine) /*return the routine*/\
    CLOSURE_NAME(CombatRoutineGenerator) \
    CLOSURE_ARGS(WorldView */*world*/, Action */*action*/)
#include "segautils\Closure_Decl.h"

typedef struct CombatRoutineLibrary_t CombatRoutineLibrary;

CombatRoutineLibrary *combatRoutineLibraryCreate();
void combatRoutineLibraryDestroy(CombatRoutineLibrary *self);

void combatRoutineLibraryAdd(CombatRoutineLibrary *self, StringView name, CombatRoutineGenerator c);
CombatRoutineGenerator combatRoutineLibraryGet(CombatRoutineLibrary *self, StringView name);

void buildAllCombatRoutines(CombatRoutineLibrary *self);

CombatRoutineGenerator buildMeleeAttackRoutine();
CombatRoutineGenerator buildBowAttackRoutine();
CombatRoutineGenerator buildProjectileAttackRoutine();
CombatRoutineGenerator buildSwapAttackRoutine();
CombatRoutineGenerator buildSwapOtherAttackRoutine();
CombatRoutineGenerator buildAutoAttackRoutine();