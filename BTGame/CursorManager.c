#include "Managers.h"

#include "Managers.h"
#include "ImageLibrary.h"
#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"

#include <stdio.h>

typedef struct{
   byte flag;
}TCursorComponent;

#define TComponentT TCursorComponent
#include "Entities\ComponentDeclTransient.h"

struct CursorManager_t{
   Manager m;
   EntitySystem *system;

   vec(Int2) *dragBox;
};

ImplManagerVTable(CursorManager)

CursorManager *createCursorManager(EntitySystem *system){
   CursorManager *out = checkedCalloc(1, sizeof(CursorManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(CursorManager);

   out->dragBox = vecCreate(Int2)(NULL);
   vecResize(Int2)(out->dragBox, 4, &(Int2){0, 0});

   return out;
}

void _destroy(CursorManager *self){
   vecDestroy(Int2)(self->dragBox);
   checkedFree(self);
}
void _onDestroy(CursorManager *self, Entity *e){}
void _onUpdate(CursorManager *self, Entity *e){}

static void _updateDragBox(CursorManager *self, int x, int y){

   *vecAt(Int2)(self->dragBox, 0) = (Int2){ 100, 100 };
   *vecAt(Int2)(self->dragBox, 1) = (Int2){ x, 100 };
   *vecAt(Int2)(self->dragBox, 2) = (Int2){ x, y };
   *vecAt(Int2)(self->dragBox, 3) = (Int2){ 100, y };

}

void cursorManagerCreateCursor(CursorManager *self){
   Entity *e = entityCreate(self->system);

   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/cursor.ega"));
   COMPONENT_ADD(e, TCursorComponent, 0);
   COMPONENT_ADD(e, LayerComponent, LayerUI);
   COMPONENT_ADD(e, PolygonComponent, .pList = self->dragBox, .color = 15);
   entityUpdate(e);
}

void cursorManagerUpdate(CursorManager *self, int x, int y){

   _updateDragBox(self, x, y);

   COMPONENT_QUERY(self->system, TCursorComponent, tc, {
      PositionComponent *pc = entityGet(PositionComponent)(componentGetParent(tc, self->system));
      if (pc){
         pc->x = x;
         pc->y = y;
      }
   });
}
