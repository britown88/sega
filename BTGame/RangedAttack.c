#include "Abilities.h"
#include "Commands.h"
#include "Managers.h"
#include "Actions.h"

static Action *_buildRangedAttack(ClosureData d, WorldView *view, Entity *user){
   Action *a = actionCreateCustom(view->managers->commandManager);
   COMPONENT_ADD(a, ActionRoutineComponent, stringIntern("bow"));
   COMPONENT_ADD(a, ActionRangeComponent, 4.0f);
   return a;
}

AbilityGenerator buildRangedAttackAbility(){
   AbilityGenerator out;

   closureInit(AbilityGenerator)(&out, NULL, (AbilityGeneratorFunc)&_buildRangedAttack, NULL);

   return out;
}