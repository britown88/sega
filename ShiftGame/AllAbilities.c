#include "Abilities.h"

void buildAllAbilities(AbilityLibrary *self){
   abilityLibraryAdd(self, stringIntern("auto"), buildAutoAttackAbility());
   
   abilityLibraryAdd(self, stringIntern("melee"), buildMeleeAttackAbility());
   abilityLibraryAdd(self, stringIntern("move"), buildMoveAbility());
   abilityLibraryAdd(self, stringIntern("ranged"), buildRangedAttackAbility());
   abilityLibraryAdd(self, stringIntern("stun"), buildStunAbility());


   abilityLibraryAdd(self, stringIntern("swap"), buildSwapAbility());
   abilityLibraryAdd(self, stringIntern("magic_missile"), buildMagicMissileAbility());
}