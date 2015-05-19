#include "CombatRoutines.h"

void buildAllCombatRoutines(CombatRoutineLibrary *self){
   combatRoutineLibraryAdd(self, stringIntern("melee"), buildMeleeAttackRoutine());
   combatRoutineLibraryAdd(self, stringIntern("bow"), buildBowAttackRoutine());
   combatRoutineLibraryAdd(self, stringIntern("projectile"), buildProjectileAttackRoutine());
}