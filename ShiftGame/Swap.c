#include "Abilities.h"
#include "Commands.h"
#include "Managers.h"
#include "Actions.h"

static Action *_buildSwap(ClosureData d, WorldView *view, Entity *user){
   Action *a = actionCreateCustom(view->managers->commandManager);
   COMPONENT_ADD(a, ActionRoutineComponent, stringIntern("swap"));
   COMPONENT_ADD(a, ActionRangeComponent, 10.0f);
   return a;
}

AbilityGenerator buildSwapAbility(){
   AbilityGenerator out;

   closureInit(AbilityGenerator)(&out, NULL, (AbilityGeneratorFunc)&_buildSwap, NULL);

   return out;
}