#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"
#include "Combat.h"
#include "CombatConstants.h"
#include "LogManager.h"
#include "SelectionManager.h"
#include "StatusManager.h"
#include "GridManager.h"

#include <math.h>

#define ComponentT StatsComponent
#include "Entities\ComponentImpl.h"
#define ComponentT StatModsComponent
#include "Entities\ComponentImpl.h"

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
#define ComponentT CActionInflictsStatusComponent
#include "Entities\ComponentImpl.h"
#define ComponentT CActionRemovesStatusComponent
#include "Entities\ComponentImpl.h"

#define HEALTH_BAR_WIDTH GRID_RES_SIZE
#define HEALTH_BAR_HEIGHT 2

typedef struct{
   bool isDead;
   Entity *healthbar_bg;
   Entity *healthbar_fg;
}TCombatComponent;

static void TCombatComponentDestroy(TCombatComponent *tcc){
   COMPONENT_ADD(tcc->healthbar_bg, DestructionComponent, 0);
   COMPONENT_ADD(tcc->healthbar_fg, DestructionComponent, 0);
}

#define COMP_DESTROY_FUNC TCombatComponentDestroy
#define TComponentT TCombatComponent
#include "Entities\ComponentDeclTransient.h"

struct CombatManager_t{
   Manager m;
   WorldView *view;
   EntitySystem *actions;
   vec(EntityPtr) *deadList;
};

ImplManagerVTable(CombatManager)

static void _statsComponentUpdate(CombatManager *self, Entity *e, StatsComponent *oldSC);
static void _statModsComponentUpdate(CombatManager *self, Entity *e, StatModsComponent *oldSMC);

static void _registerUpdateDelegates(CombatManager *self){
   ComponentUpdate statsUpdate, modsUpdate;

   closureInit(ComponentUpdate)(&statsUpdate, self, (ComponentUpdateFunc)&_statsComponentUpdate, NULL);
   compRegisterUpdateDelegate(StatsComponent)(self->view->entitySystem, statsUpdate);

   closureInit(ComponentUpdate)(&modsUpdate, self, (ComponentUpdateFunc)&_statModsComponentUpdate, NULL);
   compRegisterUpdateDelegate(StatModsComponent)(self->view->entitySystem, modsUpdate);
}

static void _updateDeadEntity(EntityPtr *e){
   TCombatComponent *tc = entityGet(TCombatComponent)(*e);

   COMPONENT_ADD(tc->healthbar_bg, VisibilityComponent, .shown = false);
   COMPONENT_ADD(tc->healthbar_fg, VisibilityComponent, .shown = false);

   COMPONENT_ADD(*e, VisibilityComponent, .shown = false);
   entityRemove(GridComponent)(*e);
   entityDeselect(*e);
   entityUpdate(*e);
}

CombatManager *createCombatManager(WorldView *view){
   CombatManager *out = checkedCalloc(1, sizeof(CombatManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(CombatManager);

   out->actions = entitySystemCreate();
   _registerUpdateDelegates(out);

   out->deadList = vecCreate(EntityPtr)(&_updateDeadEntity);

   return out;
}

static void _addHealthbars(CombatManager *self, Entity *e){
   TCombatComponent *tc = entityGet(TCombatComponent)(e);

   tc->healthbar_bg = entityCreate(self->view->entitySystem);
   COMPONENT_ADD(tc->healthbar_bg, PositionComponent, 0, 0);
   COMPONENT_ADD(tc->healthbar_bg, RectangleComponent, 4);
   COMPONENT_ADD(tc->healthbar_bg, LayerComponent, LayerPostToken0);
   COMPONENT_ADD(tc->healthbar_bg, SizeComponent, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT);
   COMPONENT_ADD(tc->healthbar_bg, LockedPositionComponent, e, 0, GRID_RES_SIZE - HEALTH_BAR_HEIGHT);
   entityUpdate(tc->healthbar_bg);

   tc->healthbar_fg = entityCreate(self->view->entitySystem);
   COMPONENT_ADD(tc->healthbar_fg, PositionComponent, 0, 0);
   COMPONENT_ADD(tc->healthbar_fg, RectangleComponent, 2);
   COMPONENT_ADD(tc->healthbar_fg, LayerComponent, LayerPostToken1);
   COMPONENT_ADD(tc->healthbar_fg, SizeComponent, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT);
   COMPONENT_ADD(tc->healthbar_fg, LockedPositionComponent, e, 0, GRID_RES_SIZE - HEALTH_BAR_HEIGHT);
   entityUpdate(tc->healthbar_fg);
}

void _destroy(CombatManager *self){
   entitySystemDestroy(self->actions);
   vecDestroy(EntityPtr)(self->deadList);
   checkedFree(self);
}
void _onDestroy(CombatManager *self, Entity *e){}
void _onUpdate(CombatManager *self, Entity *e){
   StatsComponent *sc = entityGet(StatsComponent)(e);
   TCombatComponent *tcc = entityGet(TCombatComponent)(e);

   if (sc){
      if (!tcc){
         COMPONENT_ADD(e, TCombatComponent, .isDead = false);
         _addHealthbars(self, e);
      }
   }
   else{
      if (tcc){
         entityRemove(TCombatComponent)(e);
      }
   }
}

void _statsComponentUpdate(CombatManager *self, Entity *e, StatsComponent *oldSC){
   StatsComponent *sc = entityGet(StatsComponent)(e);
   TCombatComponent *tcc = entityGet(TCombatComponent)(e);

   if (sc && tcc){
      if (sc->HP <= 0.0f){
         tcc->isDead = true;
      }
   }
}

void _statModsComponentUpdate(CombatManager *self, Entity *e, StatModsComponent *oldSMC){
   StatModsComponent *smc = entityGet(StatModsComponent)(e);

}

static void _updateHealthbars(CombatManager *self, Entity *e){
   StatsComponent *sc = entityGet(StatsComponent)(e);
   TCombatComponent *tcc = entityGet(TCombatComponent)(e);

   entityGet(SizeComponent)(tcc->healthbar_fg)->x = (int)((sc->HP / entityGetMaxHP(e)) * (float)HEALTH_BAR_WIDTH);
}

void combatManagerUpdate(CombatManager *self){ 

   COMPONENT_QUERY(self->view->entitySystem, TCombatComponent, tc, {
      Entity *e = componentGetParent(tc, self->view->entitySystem);
      if (tc->isDead && entityGet(GridComponent)(e)){
         vecPushBack(EntityPtr)(self->deadList, &e);
      }

      if (!tc->isDead){
         _updateHealthbars(self, e);
      }
   });

   vecClear(EntityPtr)(self->deadList);
}

//create a new combat action
CombatAction *combatManagerCreateAction(CombatManager *self, Entity *source, Entity *target){
   CombatAction *out = entityCreate(self->actions);
   COMPONENT_ADD(out, CActionSourceComponent, .source = source);
   COMPONENT_ADD(out, CActionTargetComponent, .target = target);
   return out;
}

CombatAction *combatActionCreateCustom(CombatManager *self){
   CombatAction *out = entityCreate(self->actions);
   return out;
}

//declare your intent, returns the modified action to perform
CombatAction *combatManagerDeclareAction(CombatManager *self, CombatAction *proposed){
   Entity *src = entityGet(CActionSourceComponent)(proposed)->source;
   Entity *tar = entityGet(CActionTargetComponent)(proposed)->target;
   
   return proposed;
}

//query the final action at the time of infliction, returns the modified result to apply
CombatAction *combatManagerQueryActionResult(CombatManager *self, CombatAction *proposed){
   Entity *src = entityGet(CActionSourceComponent)(proposed)->source;
   Entity *tar = entityGet(CActionTargetComponent)(proposed)->target;

   CActionDamageComponent *dc = entityGet(CActionDamageComponent)(proposed);
   CActionDamageTypeComponent *dtc = entityGet(CActionDamageTypeComponent)(proposed);
   CActionRangeComponent *rc = entityGet(CActionRangeComponent)(proposed);

   if (dc){
      float arm = 0.0f;
      //logManagerPushMessage(self->view->managers->logManager, "Attack for %0.2f!", dc->damage);

      if (dtc){         
         if (dtc->type == DamageTypePhysical){
            dc->damage *= 1.0f + entityGetStrength(src) * COMBAT_PHY_PER_STR;
         }
         else if (dtc->type == DamageTypeMagical){
            dc->damage *= 1.0f + entityGetIntelligence(src) * COMBAT_MAG_PER_INT;
         }
      }

      //variance
      dc->damage *= 1.0f + (appRand(appGet(), 0, 10) - 5) * 0.01f;

      arm = (float)(int)entityGetArmor(tar);
      if (arm > 0.0f){
         arm = 1.0f - (COMBAT_ARMOR_REDUCTION * arm) / (1.0f + COMBAT_ARMOR_REDUCTION * arm);
      }
      else{
         arm = 2.0f - powf(1.0f - COMBAT_ARMOR_REDUCTION, -arm);
      }

      dc->damage *= arm;

   }

   return proposed;
}

static void _createDamageMarker(Entity *target, int damage){
   PositionComponent *pc = entityGet(PositionComponent)(target);
   SizeComponent *sc = entityGet(SizeComponent)(target);
   Entity *dmg = entityCreate(entityGetSystem(target));
   COMPONENT_ADD(dmg, PositionComponent, pc->x + (sc->x / 2), pc->y + (sc->y / 2));
   COMPONENT_ADD(dmg, DamageMarkerComponent, damage);
   entityUpdate(dmg);
}

void combatManagerExecuteAction(CombatManager *self, CombatAction *action){
   Entity *src = entityGet(CActionSourceComponent)(action)->source;
   Entity *tar = entityGet(CActionTargetComponent)(action)->target;
   CActionDamageComponent *dc = entityGet(CActionDamageComponent)(action);

   if (dc){
      COMPONENT_LOCK(StatsComponent, sc, tar, {
         sc->HP -= dc->damage;
      });
      _createDamageMarker(tar, (int)dc->damage);
      //logManagerPushMessage(self->view->managers->logManager, "Hit for %0.2f!", dc->damage);
   }

   IF_COMPONENT(action, CActionInflictsStatusComponent, isc, {
      entityAddStatus(self->view->managers->statusManager, tar, isc->status);
   });

   entityDestroy(action);
}
bool entitiesAreEnemies(Entity *e1, Entity *e2){
   TeamComponent *tc1 = entityGet(TeamComponent)(e1);
   TeamComponent *tc2 = entityGet(TeamComponent)(e2);

   if (tc1 && tc2){
      return tc1->teamID != tc2->teamID;
   }

   return false;
}

bool entityIsDead(Entity *e){
   TCombatComponent *tcc = entityGet(TCombatComponent)(e);

   if (tcc){
      return tcc->isDead;
   }

   return false;
}

float entityGetMaxHP(Entity *e){
   StatsComponent *sc = entityGet(StatsComponent)(e);
   float out = 0.0;
   if (sc){
      StatModsComponent *smc = entityGet(StatModsComponent)(e);
      float str = sc->strength;

      if (smc){
         str += smc->strength;
      }

      out += COMBAT_BASE_HP;
      out += str * COMBAT_HP_PER_STR;

      if (smc){
         out += smc->HP;
      }
   }

   return out;
}
float entityGetStrength(Entity *e){
   StatsComponent *sc = entityGet(StatsComponent)(e);
   float out = 0.0;
   if (sc){
      StatModsComponent *smc = entityGet(StatModsComponent)(e);
      out = sc->strength;

      if (smc){
         out += smc->strength;
      }
   }

   return out;
}
float entityGetAgility(Entity *e){
   StatsComponent *sc = entityGet(StatsComponent)(e);
   float out = 0.0;
   if (sc){
      StatModsComponent *smc = entityGet(StatModsComponent)(e);
      out = sc->agility;

      if (smc){
         out += smc->agility;
      }
   }

   return out;
}
float entityGetIntelligence(Entity *e){
   StatsComponent *sc = entityGet(StatsComponent)(e);
   float out = 0.0;
   if (sc){
      StatModsComponent *smc = entityGet(StatModsComponent)(e);
      out = sc->intelligence;

      if (smc){
         out += smc->intelligence;
      }
   }

   return out;
}
float entityGetArmor(Entity *e){
   StatsComponent *sc = entityGet(StatsComponent)(e);
   float out = 0.0;
   if (sc){
      StatModsComponent *smc = entityGet(StatModsComponent)(e);
      float agi = sc->agility;

      if (smc){
         agi += smc->agility;
      }

      out += agi * COMBAT_ARM_PER_AGI;

      if (smc){
         out += smc->armor;
      }
   }

   return out;
}

