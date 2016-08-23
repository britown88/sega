#include "Managers.h"
#include "Verbs.h"
#include "WorldView.h"
#include "segashared/CheckedMemory.h"
#include "segautils/Defs.h"
#include "Lua.h"
#include "GridManager.h"
#include "ImageLibrary.h"
#include "segalib/EGA.h"

#define BTN_SIZE 22
#define BTN_SPACE 1
#define BTN_TOP 175
#define BTN_LEFT 16


typedef struct  {
   Int2 pos, imgPos;
}Verb;

struct VerbManager_t {
   WorldView *view;
   Verbs focus, current;
   bool enabled;
   StringView atlas;
   ManagedImage *atlasImg;

   Verb verbs[Verb_COUNT];
};

static void _createVerbs(VerbManager *self) {
   int i;
   self->atlas = stringIntern(IMG_VERBS);
   for (i = 0; i < Verb_COUNT; ++i) {
      int x = BTN_LEFT + ((BTN_SIZE + BTN_SPACE) * i);
      int y = BTN_TOP;
      int imgX = BTN_SIZE * i;
      int imgY = 0;

      self->verbs[i].pos = (Int2) { x, y };
      self->verbs[i].imgPos = (Int2) { imgX, imgY };
   }
}


VerbManager *verbManagerCreate(WorldView *view) {
   VerbManager *out = checkedCalloc(1, sizeof(VerbManager));
   out->view = view;
   out->focus = out->current = Verb_COUNT;
   out->enabled = true;

   _createVerbs(out);

   return out;
}

void verbManagerDestroy(VerbManager *self) {

   if (self->atlasImg) {
      managedImageDestroy(self->atlasImg);
   }

   checkedFree(self);
}

typedef enum {
   Pressed = 0,
   Released
}VerbActions;

void _btnState(VerbManager *self, Verbs v, VerbActions action) {
   self->verbs[v].imgPos.y = (action == Pressed ? BTN_SIZE : 0);

   if (action == Released) {
      cursorManagerSetVerb(self->view->cursorManager, v);
      self->current = v;
   }
}

void verbManagerSetEnabled(VerbManager *self, bool enabled) {
   self->enabled = enabled;
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

   if (!self->enabled) {
      return 0;
   }

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
            Actor *a = gridManagerActorFromScreenPosition(self->view->gridManager, e->pos);
            if (a) {
               luaActorInteract(self->view->L, a, self->current);
            }
            self->current = Verb_COUNT;
            cursorManagerClearVerb(self->view->cursorManager);
            return 1;
         }
      }
   }  

   return 0;
}
void verbManagerKeyButton(VerbManager *self, Verbs v, SegaKeyActions action) {

   if (!self->enabled) {
      return;
   }
   
   if (action == SegaKey_Pressed) {
      _btnState(self, v, Pressed);
   }
   else if (action == SegaKey_Released) {
      _btnState(self, v, Released);
   }
   
}

void verbManagerRender(VerbManager *self, Frame *frame) {
   if (self->enabled) {
      int i;
      if (!self->atlasImg) {
         self->atlasImg = imageLibraryGetImage(self->view->imageLibrary, self->atlas);
      }

      for (i = 0; i < Verb_COUNT; ++i) {
         frameRenderImagePartial(frame, FrameRegionFULL,
            self->verbs[i].pos.x, self->verbs[i].pos.y,
            managedImageGetImage(self->atlasImg),
            self->verbs[i].imgPos.x, self->verbs[i].imgPos.y, 
            BTN_SIZE, BTN_SIZE);
      }
   }
}