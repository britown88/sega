#include "Abilities.h"
#include "Commands.h"
#include "Managers.h"
#include "Actions.h"

static Action *_buildAutoAttack(ClosureData d, WorldView *view, Entity *user){
   Action *a = actionCreateCustom(view->managers->commandManager);
   COMPONENT_ADD(a, ActionRoutineComponent, stringIntern("auto"));
   return a;
}

AbilityGenerator buildAutoAttackAbility(){
   AbilityGenerator out;

   closureInit(AbilityGenerator)(&out, NULL, (AbilityGeneratorFunc)&_buildAutoAttack, NULL);

   return out;
}