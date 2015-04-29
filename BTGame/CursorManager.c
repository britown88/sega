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
};

ImplManagerVTable(CursorManager)

CursorManager *createCursorManager(EntitySystem *system){
   CursorManager *out = checkedCalloc(1, sizeof(CursorManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(CursorManager);
   return out;
}

void _destroy(CursorManager *self){
   checkedFree(self);
}
void _onDestroy(CursorManager *self, Entity *e){}
void _onUpdate(CursorManager *self, Entity *e){}

void cursorManagerCreateCursor(CursorManager *self){
   Entity *e = entityCreate(self->system);

   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/cursor.ega"));
   COMPONENT_ADD(e, TCursorComponent, 0);
   COMPONENT_ADD(e, LayerComponent, LayerUI);
   entityUpdate(e);
}

void cursorManagerUpdate(CursorManager *self, int x, int y){
   COMPONENT_QUERY(self->system, TCursorComponent, tc, {
      PositionComponent *pc = entityGet(PositionComponent)(componentGetParent(tc, self->system));
      if (pc){
         pc->x = x;
         pc->y = y;
      }
   });
}
