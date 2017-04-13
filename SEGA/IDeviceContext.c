#include "IDeviceContext.h"

int iDeviceContextInit(IDeviceContext *self, int width, int height, StringView winTitle, int flags){
   return self->vTable->init(self, width, height, winTitle, flags);
}
void iDeviceContextInitRendering(IDeviceContext *self) {
   self->vTable->initRendering(self);
}
void iDeviceContextCommitRender(IDeviceContext *self) {
   self->vTable->commitRender(self);
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
Microseconds iDeviceContextTime(IDeviceContext *self){
   return self->vTable->time(self);
}
Microseconds iDeviceContextTimeMS(IDeviceContext *self) {
   return self->vTable->timeMS(self);
}
void iDeviceContextDestroy(IDeviceContext *self){
   self->vTable->destroy(self);
}
Keyboard *iDeviceContextKeyboard(IDeviceContext *self){
   return self->vTable->keyboard(self);
}
Mouse *iDeviceContextMouse(IDeviceContext *self){
   return self->vTable->mouse(self);
}
