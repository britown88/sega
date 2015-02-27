#include "IDeviceContext.h"

int iDeviceContextInit(IDeviceContext *self, int width, int height, StringView winTitle, int flags){
   return self->vTable->init(self, width, height, winTitle, flags);
}
void iDeviceContextPreRender(IDeviceContext *self){
   self->vTable->preRender(self);
}
void iDeviceContextPostRender(IDeviceContext *self){
   self->vTable->postRender(self);
}
int iDeviceContextShouldClose(IDeviceContext *self) {
   return self->vTable->shouldClose(self);
}
Int2 iDeviceContextWindowSize(IDeviceContext *self){
   return self->vTable->windowSize(self);
}
double iDeviceContextTime(IDeviceContext *self){
   return self->vTable->time(self);
}
void iDeviceContextDestroy(IDeviceContext *self){
   self->vTable->destroy(self);
}
