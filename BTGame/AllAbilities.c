#include "Abilities.h"

void buildAllAbilities(AbilityLibrary *self){
   abilityLibraryAdd(self, stringIntern("magic-missile"), buildMagicMissileAbility());
}