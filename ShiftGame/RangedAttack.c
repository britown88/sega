#include "Abilities.h"
#include "Commands.h"
#include "Managers.h"
#include "Actions.h"
#include "Combat.h"
#include "CoreComponents.h"
#include "StatusManager.h"

static Action *_buildRangedAttack(ClosureData d, WorldView *view, Entity *user){
   float range = 4.0f;
   ShiftManagers *managers = view->managers;   

   CombatAction *dmg = combatActionCreateCustom(managers->combatManager);
   COMPONENT_ADD(dmg, CActionSourceComponent, .source = user);
   COMPONENT_ADD(dmg, CActionDamageComponent, .damage = 30.0f);
   COMPONENT_ADD(dmg, CActionRangeComponent, .range = range);
   COMPONENT_ADD(dmg, CActionDamageTypeComponent, .type = DamageTypePhysical);

   Action *subAction = actionCreateCustom(view->managers->commandManager);
   COMPONENT_ADD(subAction, ImageComponent, stringIntern("assets/img/dota/venge.ega"));
   COMPONENT_ADD(subAction, ActionRoutineComponent, stringIntern("projectile"));
   COMPONENT_ADD(subAction, ActionDeliveryComponent, dmg);

   Action *a = actionCreateCustom(view->managers->commandManager);
   COMPONENT_ADD(a, ActionRoutineComponent, stringIntern("bow"));
   COMPONENT_ADD(a, ActionRangeComponent, range);
   COMPONENT_ADD(a, ActionPreDelayComponent, 0.775f);
   COMPONENT_ADD(a, ActionDeliveryComponent, dmg);
   COMPONENT_ADD(a, ActionSubActionComponent, subAction);

   return a;
}

AbilityGenerator buildRangedAttackAbility(){
   AbilityGenerator out;

   closureInit(AbilityGenerator)(&out, NULL, (AbilityGeneratorFunc)&_buildRangedAttack, NULL);

   return out;
}