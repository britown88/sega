#include "Managers.h"

#include "Managers.h"
#include "ImageLibrary.h"
#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "WorldView.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "segautils\Rect.h"
#include "segautils\Defs.h"

#include <stdio.h>

#define CURSOR_SIZE 22

typedef struct{
   byte flag;
}TCursorComponent;

#define TComponentT TCursorComponent
#include "Entities\ComponentDeclTransient.h"

struct CursorManager_t{
   Manager m;
   WorldView *view;
   Entity *e;
};

ImplManagerVTable(CursorManager)

CursorManager *createCursorManager(WorldView *view){
   CursorManager *out = checkedCalloc(1, sizeof(CursorManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(CursorManager);

   return out;
}

void _destroy(CursorManager *self){
   checkedFree(self);
}
void _onDestroy(CursorManager *self, Entity *e){}
void _onUpdate(CursorManager *self, Entity *e){}

void cursorManagerCreateCursor(CursorManager *self){
   self->e = entityCreate(self->view->entitySystem);

   COMPONENT_ADD(self->e, PositionComponent, 0, 0);
   COMPONENT_ADD(self->e, ImageComponent, .imgID = stringIntern(IMG_CURSOR), .partial = true, .height = CURSOR_SIZE, .width = CURSOR_SIZE, .x = 0, .y = 0);
   COMPONENT_ADD(self->e, TCursorComponent, 0);
   COMPONENT_ADD(self->e, LayerComponent, LayerCursor);
   COMPONENT_ADD(self->e, RenderedUIComponent, 0);
   entityUpdate(self->e);
}

void cursorManagerSetVerb(CursorManager *self, Verbs v) {
   ImageComponent *ic = entityGet(ImageComponent)(self->e);

   if (v < Verb_COUNT) {
      ic->x = (v + 1) * CURSOR_SIZE;
   }
   else {
      ic->x = 0;
   }
}

void cursorManagerClearVerb(CursorManager *self) {
   entityGet(ImageComponent)(self->e)->x = 0;
}

void cursorManagerUpdate(CursorManager *self, int x, int y){
   PositionComponent *pc = entityGet(PositionComponent)(self->e);
   if (pc){
      pc->x = x;
      pc->y = y;
   }
}
