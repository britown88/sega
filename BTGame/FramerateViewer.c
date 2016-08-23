#include "Managers.h"
#include "WorldView.h"
#include "segalib/EGA.h"
#include "segashared/CheckedMemory.h"


struct FramerateViewer_t {
   WorldView *view;
   double *fps;
   bool show;
};

FramerateViewer *framerateViewerCreate(WorldView *view, double *fps) {
   FramerateViewer *out = checkedCalloc(1, sizeof(FramerateViewer));
   out->fps = fps;
   out->view = view;

#ifdef _DEBUG
   out->show = true;
#else
   out->show = false;
#endif
   return out;
}
void framerateViewerDestroy(FramerateViewer *self) {
   checkedFree(self);
}
void framerateViewerToggle(FramerateViewer *self) {
   self->show = !self->show;
}
void framerateViewerRender(FramerateViewer *self, Frame *frame) {
   if (self->show) {
      static char buffer[256] = { 0 };
      short y = 2;
      short x;
      sprintf(buffer, "FPS: %.2f", *self->fps);

      x = EGA_TEXT_RES_WIDTH - (byte)strlen(buffer) - 2;
      frameRenderText(frame, buffer, x, y, fontFactoryGetFont(self->view->fontFactory, 0, 15));
   }
}