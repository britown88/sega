#include "IRenderer.h"

void iRendererInit(IRenderer*self){
   if (self){
      self->vTable->init(self);
   }
}
void iRendererRenderFrame(IRenderer*self, Frame *frame, byte *palette, Rectf *viewport){
   if (self){
      self->vTable->renderFrame(self, frame, palette, viewport);
   }
}
void iRendererDestroy(IRenderer*self){
   if (self){
      self->vTable->destroy(self);
   }
}