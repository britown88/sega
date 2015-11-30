#include "Managers.h"
#include "Verbs.h"
#include "Entities/Entities.h"
#include "CoreComponents.h"
#include "WorldView.h"
#include "segashared/CheckedMemory.h"
#include "segautils/Defs.h"
#include "Lua.h"
#include "GridManager.h"

#define BTN_SIZE 22
#define BTN_SPACE 1
#define BTN_TOP 175
#define BTN_LEFT 16

struct VerbManager_t {
   Manager m;
   WorldView *view;
   Entity *buttons[Verb_COUNT];
   Verbs focus, current;
};

ImplManagerVTable(VerbManager)

VerbManager *createVerbManager(WorldView *view) {
   VerbManager *out = checkedCalloc(1, sizeof(VerbManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(VerbManager);
   out->focus = out->current = Verb_COUNT;
   return out;
}

void _destroy(VerbManager *self) {
   checkedFree(self);
}
void _onDestroy(VerbManager *self, Entity *e) {}
void _onUpdate(VerbManager *self, Entity *e) {}

typedef enum {
   Pressed = 0,
   Released
}VerbActions;

void _btnState(VerbManager *self, Verbs v, VerbActions action) {
   ImageComponent *ic = entityGet(ImageComponent)(self->buttons[v]);
   ic->y = action == Pressed ? BTN_SIZE : 0;

   if (action == Released) {
      cursorManagerSetVerb(self->view->managers->cursorManager, v);
      self->current = v;
   }
}

void verbManagerCreateVerbs(VerbManager *self) {
   int i;
   StringView atlas = stringIntern("assets/img/verbs.ega");
   for (i = 0; i < Verb_COUNT; ++i) {
      int x = BTN_LEFT + ((BTN_SIZE + BTN_SPACE) * i);
      int y = BTN_TOP;
      int imgX = BTN_SIZE * i;
      int imgY = 0;

      Entity *e = entityCreate(self->view->entitySystem);
      COMPONENT_ADD(e, PositionComponent, x, y);
      COMPONENT_ADD(e, SizeComponent, BTN_SIZE, BTN_SIZE);
      COMPONENT_ADD(e, ImageComponent, .filename = atlas, .partial = true, .x = imgX, .y = imgY, .width = BTN_SIZE, .height = BTN_SIZE);
      COMPONENT_ADD(e, LayerComponent, LayerUI);
      COMPONENT_ADD(e, RenderedUIComponent, 0);
      entityUpdate(e);
      
      self->buttons[i] = e;
   }
}
int verbManagerMouseButton(VerbManager *self, MouseEvent *e) {
   Viewport *vp = self->view->viewport;
   Recti vpArea = { 
      vp->region.origin_x, 
      vp->region.origin_y, 
      vp->region.origin_x + vp->region.width, 
      vp->region.origin_y + vp->region.height 
   };

   static Recti btnArea = {
      BTN_LEFT, 
      BTN_TOP, 
      BTN_LEFT + ((BTN_SIZE + BTN_SPACE) * Verb_COUNT), 
      BTN_TOP + BTN_SIZE 
   };

   Recti singleArea = {
      BTN_LEFT,
      BTN_TOP,
      BTN_LEFT + BTN_SIZE,
      BTN_TOP + BTN_SIZE
   };

   int i;

   if (self->focus < Verb_COUNT) {      
      if (e->action == SegaMouse_Released) {
         _btnState(self, self->focus, Released);
         self->focus = Verb_COUNT;
         return 1;
      }
      else {
         self->focus = Verb_COUNT;
         return 0;
      }
   }

   if (rectiContains(btnArea, e->pos)) {
      for (i = 0; i < Verb_COUNT; ++i) {
         if (rectiContains(singleArea, e->pos)) {
            if (e->action == SegaMouse_Pressed) {
               _btnState(self, i, Pressed);
               self->focus = i;
            }
            else if (e->action == SegaMouse_Released) {
               _btnState(self, i, Released);
            }
            else {
               //we were given an action that we dont care about
               return 0;
            }
            return 1;
         }

         singleArea.left += BTN_SIZE + BTN_SPACE;
         singleArea.right += BTN_SIZE + BTN_SPACE;
      }
   }
   else if (rectiContains(vpArea, e->pos)) {
      if (e->action == SegaMouse_Released) {
         if (self->current < Verb_COUNT) {
            Entity *entity = gridMangerEntityFromScreenPosition(self->view->managers->gridManager, e->pos);
            if (entity) {
               luaActorInteract(self->view->L, entity, self->current);
            }
            self->current = Verb_COUNT;
            cursorManagerClearVerb(self->view->managers->cursorManager);
            return 1;
         }
      }
   }  

   return 0;
}
void verbManagerKeyButton(VerbManager *self, Verbs v, SegaKeyActions action) {
   
   if (action == SegaKey_Pressed) {
      _btnState(self, v, Pressed);
   }
   else if (action == SegaKey_Released) {
      _btnState(self, v, Released);
   }
   
}