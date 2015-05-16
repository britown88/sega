#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"
#include "Combat.h"

#define ComponentT CActionSourceComponent
#include "Entities\ComponentImpl.h"
#define ComponentT CActionTargetComponent
#include "Entities\ComponentImpl.h"
#define ComponentT CActionCancelledComponent
#include "Entities\ComponentImpl.h"
#define ComponentT CActionRangeComponent
#include "Entities\ComponentImpl.h"
#define ComponentT CActionDamageComponent
#include "Entities\ComponentImpl.h"
#define ComponentT CActionDamageTypeComponent
#include "Entities\ComponentImpl.h"

struct CombatManager_t{
   Manager m;
   WorldView *view;
   EntitySystem *actions;
};

ImplManagerVTable(CombatManager)

CombatManager *createCombatManager(WorldView *view){
   CombatManager *out = checkedCalloc(1, sizeof(CombatManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(CombatManager);

   out->actions = entitySystemCreate();

   return out;
}

void _destroy(CombatManager *self){
   entitySystemDestroy(self->actions);

   checkedFree(self);
}
void _onDestroy(CombatManager *self, Entity *e){}
void _onUpdate(CombatManager *self, Entity *e){}

//create a new combat action
CombatAction *combatManagerCreateAction(CombatManager *self, Entity *source, Entity *target){
   CombatAction *out = entityCreate(self->actions);
   COMPONENT_ADD(out, CActionSourceComponent, .source = source);
   COMPONENT_ADD(out, CActionTargetComponent, .target = target);
   return out;
}

//declare your intent, returns the modified action to perform
CombatAction *combatManagerDeclareAction(CombatManager *self, CombatAction *proposed){
   return proposed;
}

//query the final action at the time of infliction, returns the modified result to apply
CombatAction *combatManagerQueryActionResult(CombatManager *self, CombatAction *proposed){
   return proposed;
}

