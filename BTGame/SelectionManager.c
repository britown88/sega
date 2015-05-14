#include "SelectionManager.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"

//marks en entity as a transient element 
//tied to a parent
typedef struct{
   Entity *parent;
}TSelectedTransientComponent;

#define TComponentT TSelectedTransientComponent
#include "Entities\ComponentDeclTransient.h"

//alloc this vec on creation, it gets killed and cleanup by the destroy func below
typedef struct{
   vec(EntityPtr) *transientList;
}TSelectedComponent;

void TSelectedComponentDestroy(TSelectedComponent *self){
   vecForEach(EntityPtr, te, self->transientList, {
      entityGet(TSelectedTransientComponent)(*te)->parent = NULL;
      entityDestroy(*te);
   });

   vecDestroy(EntityPtr)(self->transientList);
}

#define COMP_DESTROY_FUNC TSelectedComponentDestroy
#define TComponentT TSelectedComponent
#include "Entities\ComponentDeclTransient.h"

struct SelectionManager_t{
   Manager m;
   EntitySystem *system;
   vec(EntityPtr) *selectList, *tempList;
};

ImplManagerVTable(SelectionManager)

SelectionManager *createSelectionManager(EntitySystem *system){
   SelectionManager *out = checkedCalloc(1, sizeof(SelectionManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(SelectionManager);

   out->selectList = vecCreate(EntityPtr)(NULL);
   out->tempList = vecCreate(EntityPtr)(NULL);
   return out;
}

void _destroy(SelectionManager *self){
   vecDestroy(EntityPtr)(self->selectList);
   vecDestroy(EntityPtr)(self->tempList);
   checkedFree(self);
}
void _onDestroy(SelectionManager *self, Entity *e){
   TSelectedComponent *sc = entityGet(TSelectedComponent)(e);
   TSelectedTransientComponent *scc = entityGet(TSelectedTransientComponent)(e);

   //selected entity is being deleted, remove it from our selectList
   if (sc){      
      vecRemove(EntityPtr)(self->selectList, &e);
   }

   //a transient elemnt is being desstroyed, if it still has a parent we need to 
   //remove it from its parent's list.  
   if (scc && scc->parent){
      TSelectedComponent *parentsc = entityGet(TSelectedComponent)(scc->parent);
      if (parentsc){
         vecRemove(EntityPtr)(parentsc->transientList, &e);
      }
   }
}
void _onUpdate(SelectionManager *self, Entity *e){}


//ensure selected entity has a selection list and the primary cursor transient set to shown by default
static TSelectedComponent *_initSelection(Entity *e){
   TSelectedComponent *sc = entityGet(TSelectedComponent)(e);
   if (!sc){
      Entity *cursor = entityCreate(entityGetSystem(e));

      COMPONENT_ADD(cursor, PositionComponent, 0, 0);
      COMPONENT_ADD(cursor, ImageComponent, stringIntern("assets/img/select.ega"));
      COMPONENT_ADD(cursor, RectangleComponent, 15);

      COMPONENT_ADD(cursor, LayerComponent, LayerSubToken0);;
      COMPONENT_ADD(cursor, SizeComponent, 32, 32);
      COMPONENT_ADD(cursor, LockedPositionComponent, e);
      COMPONENT_ADD(cursor, VisibilityComponent, .shown = false);
      COMPONENT_ADD(cursor, TSelectedTransientComponent, e);

      //add a tracking transient to the selected entity
      COMPONENT_ADD(e, TSelectedComponent, .transientList = vecCreate(EntityPtr)(NULL));

      entityUpdate(cursor);

      sc = entityGet(TSelectedComponent)(e);
      vecPushBack(EntityPtr)(sc->transientList, &cursor);
   }

   return sc;
}

static void _setVisibility(Entity *e, bool value){
   VisibilityComponent *vc = entityGet(VisibilityComponent)(e);

   if (!vc){
      COMPONENT_ADD(e, VisibilityComponent, .shown = value);
   }
   else{
      vc->shown = value;
   }
}

static void _hideAllTransients(SelectionManager *self){
   COMPONENT_QUERY(self->system, TSelectedTransientComponent, cc, {
      Entity *e = componentGetParent(cc, self->system);
      _setVisibility(e, false);
   });
}

void entityLinkSelectionTransient(Entity *parent, Entity *transient){
   TSelectedComponent *sc = _initSelection(parent);

   _setVisibility(transient, true);
   COMPONENT_ADD(transient, TSelectedTransientComponent, parent);
   vecPushBack(EntityPtr)(sc->transientList, &transient);
}

static bool _select(SelectionManager *self, Entity *e, SelectCriteria *filters, size_t filterCount){   
   size_t i = 0;

   for (i = 0; i < filterCount; ++i){
      SelectCriteria *f = filters + i;
      switch (f->type){
      case scArea:
      {
         PositionComponent *pc = entityGet(PositionComponent)(e);
         SizeComponent *sc = entityGet(SizeComponent)(e);
         if (pc && sc){
            Recti ebox = { pc->x, pc->y, pc->x + sc->x, pc->y + sc->y };
            if (rectiIntersects(ebox, f->box)){
               continue;
            }
         }
      }
         break;
      case scTeam:
      {
         TeamComponent *tc = entityGet(TeamComponent)(e);
         if (tc){
            if (tc->teamID == f->teamID){
               continue;
            }
         }
      }
         break;
      }

      //didnt hit a continue, so we failed
      return false;
   }

   //made it through the loop
   return true;
}

void selectionManagerSelectEx(SelectionManager *self, SelectCriteria *filters, size_t filterCount){

   vecClear(EntityPtr)(self->selectList);
   _hideAllTransients(self);

   COMPONENT_QUERY(self->system, GridComponent, tc, {
      Entity *e = componentGetParent(tc, self->system);
      if (_select(self, e, filters, filterCount)){
         vecPushBack(EntityPtr)(self->selectList, &e);
      }
   });

   vecForEach(EntityPtr, selected, self->selectList, {
      TSelectedComponent *sc = _initSelection(*selected);
      vecForEach(EntityPtr, t, sc->transientList, {
         _setVisibility(*t, true);
      });
   });
}

vec(EntityPtr) *selectionManagerGetSelected(SelectionManager *self){
   return self->selectList;
}

vec(EntityPtr) *selectionManagerGetEntitiesEX(SelectionManager *self, SelectCriteria *filters, size_t filterCount){
   vecClear(EntityPtr)(self->tempList);

   COMPONENT_QUERY(self->system, GridComponent, tc, {
      Entity *e = componentGetParent(tc, self->system);
      if (_select(self, e, filters, filterCount)){
         vecPushBack(EntityPtr)(self->tempList, &e);
      }
   });

   return self->tempList;
}
