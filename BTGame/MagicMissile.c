#include "Abilities.h"


static void _buildMagicMissile(WorldView *view, Action *a){

}

AbilityGenerator buildMagicMissileAbility(){
   AbilityGenerator out;

   closureInit(AbilityGenerator)(&out, NULL, (AbilityGeneratorFunc)&_buildMagicMissile, NULL);

   return out;
}