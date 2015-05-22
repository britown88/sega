#include "Abilities.h"
#include "Commands.h"
#include "Managers.h"
#include "Actions.h"

static Action *_buildMeleeAttack(ClosureData d, WorldView *view, Entity *user){
   Action *a = actionCreateCustom(view->managers->commandManager);
   COMPONENT_ADD(a, ActionRoutineComponent, stringIntern("melee"));
   COMPONENT_ADD(a, ActionRangeComponent, 1.0f);
   return a;
}

AbilityGenerator buildMeleeAttackAbility(){
   AbilityGenerator out;

   closureInit(AbilityGenerator)(&out, NULL, (AbilityGeneratorFunc)&_buildMeleeAttack, NULL);

   return out;
}