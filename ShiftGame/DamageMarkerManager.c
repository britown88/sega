#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"
#include <stdio.h>

#define DMARKER_BUFFER_LEN 16
#define DMARKER_HEIGHT 7
#define DMARKER_WIDTH 6

typedef struct {
   vec(EntityPtr) *digits;
}TDamageMarkerComponent;

static void TDamageMarkerComponentDestroy(TDamageMarkerComponent *self){
   vecForEach(EntityPtr, digit, self->digits, {
      COMPONENT_ADD(*digit, DestructionComponent, 0);
   });
   vecDestroy(EntityPtr)(self->digits);
}

#define COMP_DESTROY_FUNC TDamageMarkerComponentDestroy
#define TComponentT TDamageMarkerComponent
#include "Entities\ComponentDeclTransient.h"

struct DamageMarkerManager_t{
   Manager m;
   WorldView *view;
};

static void _updateDigitEntity(Entity *e, byte value, size_t position, size_t len){
   ImageComponent *ic = entityGet(ImageComponent)(e);
   LockedPositionComponent *lpc = entityGet(LockedPositionComponent)(e);
   size_t center = len / 2;

   lpc->offsetX = (center - position) * -DMARKER_WIDTH;
   if (len % 2){//odd
      lpc->offsetX -= DMARKER_WIDTH / 2;
   }

   value = MIN(9, value);
   ic->x = value * DMARKER_WIDTH;
}

static void _renderToDigits(Entity *parent, size_t value){
   TDamageMarkerComponent *dmc = entityGet(TDamageMarkerComponent)(parent);
   static char buff[DMARKER_BUFFER_LEN] = { 0 };
   size_t len, i;
   sprintf_s(buff, DMARKER_BUFFER_LEN, "%i", value);
   len = strlen(buff);

   vecResize(EntityPtr)(dmc->digits, len, NULL);
   for (i = 0; i < len; ++i){
      EntityPtr *eptr = vecAt(EntityPtr)(dmc->digits, i);
      Entity *e;

      if (!*eptr){
         e = entityCreate(entityGetSystem(parent));
         COMPONENT_ADD(e, ImageComponent,
            .filename = stringIntern("assets//img/numbers.ega"),
            .partial = true,
            .width = DMARKER_WIDTH,
            .height = DMARKER_HEIGHT,
            .x = 0, .y = 0);

         COMPONENT_ADD(e, PositionComponent, 0);
         COMPONENT_ADD(e, LockedPositionComponent,
            .parent = parent,
            .offsetX = 0,
            .offsetY = -3);
         COMPONENT_ADD(e, LayerComponent, LayerPostTokenDmg);
         entityUpdate(e);

         *eptr = e;
      }
      else{
         e = *eptr;
      }

      _updateDigitEntity(e, buff[i] - '0', i, len);
   }
}


static void _dmgMarkerComponentUpdate(GridManager *self, Entity *e, DamageMarkerComponent *olddm){
   DamageMarkerComponent *dmc = entityGet(DamageMarkerComponent)(e);
   if (dmc->value != olddm->value){
      _renderToDigits(e, dmc->value);
   }
}

static void _registerUpdateDelegate(DamageMarkerManager *self, EntitySystem *system){
   ComponentUpdate update;

   closureInit(ComponentUpdate)(&update, self, (ComponentUpdateFunc)&_dmgMarkerComponentUpdate, NULL);
   compRegisterUpdateDelegate(DamageMarkerComponent)(system, update);
}


ImplManagerVTable(DamageMarkerManager)

DamageMarkerManager *createDamageMarkerManager(WorldView *view){
   DamageMarkerManager *out = checkedCalloc(1, sizeof(DamageMarkerManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(DamageMarkerManager);

   _registerUpdateDelegate(out, view->entitySystem);
   return out;
}

static void _addNewTransient(Entity *e){
   DamageMarkerComponent *dmc = entityGet(DamageMarkerComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   COMPONENT_ADD(e, TDamageMarkerComponent, vecCreate(EntityPtr)(NULL));
   COMPONENT_ADD(e, InterpolationComponent, pc->x, pc->y - 8, 0.25f);
   entityUpdate(e);

   _renderToDigits(e, dmc->value);
}

void _destroy(DamageMarkerManager *self){
   checkedFree(self);
}
void _onDestroy(DamageMarkerManager *self, Entity *e){}
void _onUpdate(DamageMarkerManager *self, Entity *e){
   DamageMarkerComponent *dmc = entityGet(DamageMarkerComponent)(e);
   TDamageMarkerComponent *tdmc = entityGet(TDamageMarkerComponent)(e);

   if (dmc){
      if (!tdmc){
         //create new
         _addNewTransient(e);
      }
   }
   else if(tdmc){
      //remove the transient
      entityRemove(TDamageMarkerComponent)(e);
   }
}

static void _updateEntity(Entity *e){
   if (!entityGet(InterpolationComponent)(e)){
      COMPONENT_ADD(e, DestructionComponent, 0);
   }
}

void damageMarkerManagerUpdate(DamageMarkerManager *self){
   COMPONENT_QUERY(self->view->entitySystem, TDamageMarkerComponent, dmc, {
      Entity *e = componentGetParent(dmc, self->view->entitySystem);
      _updateEntity(e);
   });
}