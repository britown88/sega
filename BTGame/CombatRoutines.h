#pragma once

#include "segautils\Coroutine.h"
#include "segashared\Strings.h"

typedef struct CombatRoutineLibrary_t CombatRoutineLibrary;

CombatRoutineLibrary *combatRoutineLibraryCreate();
void combatRoutineLibraryDestroy(CombatRoutineLibrary *self);

void combatRoutineLibraryAdd(CombatRoutineLibrary *self, StringView name, Coroutine c);
Coroutine combatRoutineLibraryGet(CombatRoutineLibrary *self, StringView name);

void buildAllCombatRoutines(CombatRoutineLibrary *self);