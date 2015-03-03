#include "Managers.h"

#include "Managers.h"
#include "ImageManager.h"

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

#pragma region vtable things
static void CursorManagerDestroy(CursorManager*);
static void CursorManagerOnDestroy(CursorManager*, Entity*);
static void CursorManagerOnUpdate(CursorManager*, Entity*);

static ManagerVTable *_createVTable(){
   static ManagerVTable *out = NULL;

   if (!out){
      out = calloc(1, sizeof(ManagerVTable));
      out->destroy = (void(*)(Manager*))&CursorManagerDestroy;
      out->onDestroy = (void(*)(Manager*, Entity*))&CursorManagerOnDestroy;
      out->onUpdate = (void(*)(Manager*, Entity*))&CursorManagerOnUpdate;
   }

   return out;
}

#pragma endregion

void cursorManagerCreateCursor(CursorManager *self){
   Entity *e = entityCreate(self->system);

   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/cursor.ega"));
   COMPONENT_ADD(e, TCursorComponent, 0);
   COMPONENT_ADD(e, LayerComponent, LayerUI);
   entityUpdate(e);
}


CursorManager *createCursorManager(EntitySystem *system){
   CursorManager *out = checkedCalloc(1, sizeof(CursorManager));
   out->system = system;
   out->m.vTable = _createVTable();
   return out;
}

void CursorManagerDestroy(CursorManager *self){
   checkedFree(self);
}

void CursorManagerOnDestroy(CursorManager *self, Entity *e){}
void CursorManagerOnUpdate(CursorManager *self, Entity *e){}

void cursorManagerUpdate(CursorManager *self, int x, int y){
   COMPONENT_QUERY(self->system, TCursorComponent, tc, {
      PositionComponent *pc = entityGet(PositionComponent)(componentGetParent(tc, self->system));
      if (pc){
         pc->x = x;
         pc->y = y;
      }
   });
}
