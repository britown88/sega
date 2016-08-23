#include "Managers.h"

#include "Managers.h"
#include "ImageLibrary.h"

#include "WorldView.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "segautils\Rect.h"
#include "segautils\Defs.h"

#include <stdio.h>

#define CURSOR_SIZE 22


struct CursorManager_t{
   WorldView *view;
   ManagedImage *cursorImg;
   Int2 pos, imgPos;

};


CursorManager *cursorManagerCreate(WorldView *view){
   CursorManager *out = checkedCalloc(1, sizeof(CursorManager));
   out->view = view;

   return out;
}

void cursorManagerDestroy(CursorManager *self){
   managedImageDestroy(self->cursorImg);
   checkedFree(self);
}

void cursorManagerCreateCursor(CursorManager *self){
   self->cursorImg = imageLibraryGetImage(self->view->imageLibrary, stringIntern(IMG_CURSOR));
}

void cursorManagerSetVerb(CursorManager *self, Verbs v) {
   if (v < Verb_COUNT) {
      self->imgPos.x = (v + 1) * CURSOR_SIZE;
   }
   else {
      self->imgPos.x = 0;
   }
}

void cursorManagerClearVerb(CursorManager *self) {
   self->imgPos.x = 0;
}

void cursorManagerUpdate(CursorManager *self, int x, int y){
   self->pos = (Int2) { x, y };
}

void cursorManagerRender(CursorManager *self, Frame *frame) {
   frameRenderImagePartial(frame, FrameRegionFULL,
      self->pos.x, self->pos.y,
      managedImageGetImage(self->cursorImg),
      self->imgPos.x, self->imgPos.y, CURSOR_SIZE, CURSOR_SIZE);
}

