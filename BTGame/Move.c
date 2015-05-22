#include "Abilities.h"
#include "Commands.h"
#include "Managers.h"
#include "Actions.h"

static Action *_buildMove(ClosureData d, WorldView *view, Entity *user){
   Action *a = actionCreateCustom(view->managers->commandManager);
   return a;
}

AbilityGenerator buildMoveAbility(){
   AbilityGenerator out;

   closureInit(AbilityGenerator)(&out, NULL, (AbilityGeneratorFunc)&_buildMove, NULL);

   return out;
}