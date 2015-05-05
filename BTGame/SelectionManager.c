#include "SelectionManager.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"

#define ClosureTPart CLOSURE_NAME(SelectionCriteria)
#include "segautils\Closure_Impl.h"

typedef struct{
   Entity *cursor;
}TSelectedComponent;

#define TComponentT TSelectedComponent
#include "Entities\ComponentDeclTransient.h"

typedef struct{
   Entity *parent;
}TSelectedCursorComponent;

#define TComponentT TSelectedCursorComponent
#include "Entities\ComponentDeclTransient.h"

struct SelectionManager_t{
   Manager m;
   EntitySystem *system;
   vec(EntityPtr) *selectList;
};

ImplManagerVTable(SelectionManager)

SelectionManager *createSelectionManager(EntitySystem *system){
   SelectionManager *out = checkedCalloc(1, sizeof(SelectionManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(SelectionManager);

   out->selectList = vecCreate(EntityPtr)(NULL);
   return out;
}

void _destroy(SelectionManager *self){
   vecDestroy(EntityPtr)(self->selectList);
   checkedFree(self);
}
void _onDestroy(SelectionManager *self, Entity *e){
   TSelectedComponent *sc = entityGet(TSelectedComponent)(e);
   TSelectedCursorComponent *scc = entityGet(TSelectedCursorComponent)(e);

   //delete cursor
   if (sc){
      if (sc->cursor){
         entityGet(TSelectedCursorComponent)(sc->cursor)->parent = NULL;
         entityDestroy(sc->cursor);
      }
   }

   //remove selection from parent
   if (scc){
      if (scc->parent){
         entityRemove(TSelectedComponent)(scc->parent);
      }
   }
}
void _onUpdate(SelectionManager *self, Entity *e){}

static void _clearLists(SelectionManager *self){

   COMPONENT_QUERY(self->system, TSelectedCursorComponent, cc, {
      Entity *e = componentGetParent(cc, self->system);
      //destroying this entity auto-calls the ondestroy function from this
      //manager which removes the tracking component from the parent
      entityDestroy(e);
   });

   vecClear(EntityPtr)(self->selectList);
}

void selectionManagerSelect(SelectionManager *self, SelectionCriteria criteria){
   
   _clearLists(self);

   COMPONENT_QUERY(self->system, GridComponent, tc, {
      Entity *e = componentGetParent(tc, self->system);
      if (closureCall(&criteria, self->system, e)){
         vecPushBack(EntityPtr)(self->selectList, &e);
      }
   });

   vecForEach(EntityPtr, selected, self->selectList, {
      Entity *cursor = entityCreate(self->system);

      COMPONENT_ADD(cursor, PositionComponent, 0, 0);
      COMPONENT_ADD(cursor, ImageComponent, stringIntern("assets/img/select.ega"));
      COMPONENT_ADD(cursor, RectangleComponent, 15);

      COMPONENT_ADD(cursor, LayerComponent, LayerSubToken0);;
      COMPONENT_ADD(cursor, SizeComponent, 32, 32);
      COMPONENT_ADD(cursor, LockedPositionComponent, *selected);
      COMPONENT_ADD(cursor, TSelectedCursorComponent, *selected);

      //add a tracking transient to the selected entity
      COMPONENT_ADD(*selected, TSelectedComponent, cursor);

      entityUpdate(cursor);
   });
}

static bool _selectDataAreaTeam(SelectDataAreaTeam *data, EntitySystem *system, Entity *e){
   PositionComponent *pc = entityGet(PositionComponent)(e);
   TeamComponent *tc = entityGet(TeamComponent)(e);
   SizeComponent *sc = entityGet(SizeComponent)(e);

   if (pc && tc && sc){
      Recti ebox = { pc->x, pc->y, pc->x + sc->x, pc->y + sc->y };
      
      return   tc->teamID == data->teamID &&
         rectiIntersects(data->box, ebox);
   }

   return false;
}

SelectionCriteria selectAreaByTeam(SelectDataAreaTeam *data){
   SelectionCriteria out;
   closureInit(SelectionCriteria)(&out, data, (SelectionCriteriaFunc)&_selectDataAreaTeam, NULL);
   return out;
}