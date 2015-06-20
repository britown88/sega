#include "Abilities.h"
#include "Commands.h"
#include "Managers.h"
#include "Actions.h"
#include "CoreComponents.h"
#include "StatusManager.h"
#include "Combat.h"

static Action *_buildMagicMissile(ClosureData d, WorldView *view, Entity *user){
   float range = 6.0f;
   ShiftManagers *managers = view->managers;

   Status *s = statusCreateByName(managers->statusManager, stringIntern("stun"));
   s = statusSetDuration(s, 2.0f);

   CombatAction *dmg = combatActionCreateCustom(managers->combatManager);
   COMPONENT_ADD(dmg, CActionSourceComponent, .source = user);
   COMPONENT_ADD(dmg, CActionDamageComponent, .damage = 80.0f);
   COMPONENT_ADD(dmg, CActionRangeComponent, .range = range);
   COMPONENT_ADD(dmg, CActionDamageTypeComponent, .type = DamageTypeMagical);
   COMPONENT_ADD(dmg, CActionInflictsStatusComponent, s);

   Action *subAction = actionCreateCustom(view->managers->commandManager);
   COMPONENT_ADD(subAction, ImageComponent, stringIntern("assets/img/dota/ursa.ega"));
   COMPONENT_ADD(subAction, ActionRoutineComponent, stringIntern("projectile"));
   COMPONENT_ADD(subAction, ActionDeliveryComponent, dmg);

   Action *a = actionCreateCustom(view->managers->commandManager);
   COMPONENT_ADD(a, ActionRoutineComponent, stringIntern("bow"));
   COMPONENT_ADD(a, ActionRangeComponent, range);
   COMPONENT_ADD(a, ActionPreDelayComponent, 0.0f);
   COMPONENT_ADD(a, ActionDeliveryComponent, dmg);
   COMPONENT_ADD(a, ActionSubActionComponent, subAction);

   return a;
}


AbilityGenerator buildMagicMissileAbility(){
   AbilityGenerator out;

   closureInit(AbilityGenerator)(&out, NULL, (AbilityGeneratorFunc)&_buildMagicMissile, NULL);

   return out;
}