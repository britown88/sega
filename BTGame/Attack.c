#include "Actions.h"
#include "Commands.h"
#include "Managers.h"

Action *createActionCombat(CommandManager *self, size_t slot, Entity *e){
   Action *a = commandManagerCreateAction(self);
   COMPONENT_ADD(a, ActionTargetEntityComponent, e);
   COMPONENT_ADD(a, ActionCombatComponent, .slot = slot);
   entityUpdate(a);

   return a;
}