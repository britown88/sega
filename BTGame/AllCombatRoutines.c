#include "CombatRoutines.h"

void buildAllCombatRoutines(CombatRoutineLibrary *self){
   combatRoutineLibraryAdd(self, stringIntern("melee"), buildMeleeAttackRoutine());
}