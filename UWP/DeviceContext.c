
#include "DeviceContext.h"

#include "SEGA\IDeviceContext.h"
#include "SEGA\App.h"

#include "segashared/CheckedMemory.h"
#include "SEGA\Input.h"
#define ClosureTPart CLOSURE_NAME(MousePos)
#include "segautils\Closure_Impl.h"


typedef struct  {
   IDeviceContext context;

   uint64_t clockFreq;
   Microseconds startTime;
}UWPContext;

static int _init(UWPContext *self, int width, int height, StringView winTitle, int flags);
static void _initRendering(UWPContext *self);
static void _commitRender(UWPContext *self);
static void _preRender(UWPContext *self);
static void _postRender(UWPContext *self);
static int _shouldClose(UWPContext *self);
static Int2 _windowSize(UWPContext *self);
static Microseconds _time(UWPContext *self);
static void _destroy(UWPContext *self);
static Keyboard* _keyboard(UWPContext *self);
static Mouse* _mouse(UWPContext *self);

static IDeviceContextVTable *_getTable() {
   static IDeviceContextVTable *out = NULL;
   if (!out) {
      out = calloc(1, sizeof(IDeviceContextVTable));
      out->init = (int(*)(IDeviceContext*, int, int, StringView, int))&_init;
      out->initRendering = (void(*)(IDeviceContext*))&_initRendering;
      out->commitRender = (void(*)(IDeviceContext*))&_commitRender;
      out->preRender = (void(*)(IDeviceContext*))&_preRender;
      out->postRender = (void(*)(IDeviceContext*))&_postRender;
      out->shouldClose = (int(*)(IDeviceContext*))&_shouldClose;
      out->windowSize = (Int2(*)(IDeviceContext*))&_windowSize;
      out->time = (Microseconds(*)(IDeviceContext*))&_time;
      out->destroy = (void(*)(IDeviceContext*))&_destroy;
      out->keyboard = (Keyboard*(*)(IDeviceContext*))&_keyboard;
      out->mouse = (Mouse*(*)(IDeviceContext*))&_mouse;
   }
   return out;
}

IDeviceContext *createUWPContext() {
   UWPContext *out = checkedCalloc(1, sizeof(UWPContext));
   out->context.vTable = _getTable();
   return (IDeviceContext *)out;
}

int _init(UWPContext *self, int width, int height, StringView winTitle, int flags) {
   Int2 size = { width, height };
   //GLFWmonitor *monitor = NULL;

   //if (!glfwInit()) {
   //   return 1;
   //}

   //if (flags&DC_FLAG_FULLSCREEN) {
   //   monitor = glfwGetPrimaryMonitor();
   //}


   //self->window = glWindowCreate(size, winTitle, monitor);

   //if (!self->window) {
   //   glfwTerminate();
   //   return 1;
   //}

   //QueryPerformanceFrequency((LARGE_INTEGER*)&self->clockFreq);
   //QueryPerformanceCounter((LARGE_INTEGER*)&self->startTime);
   return 0;
}

void _initRendering(UWPContext *self) {
   //glWindowBeginRendering(self->window);
}
void _commitRender(UWPContext *self) {
   //glWindowSwapBuffers(self->window);
}
void _preRender(UWPContext *self) {

}
void _postRender(UWPContext *self) {
   //glWindowPollEvents(self->window);
}
int _shouldClose(UWPContext *self) {
   //return glWindowShouldClose(self->window);
   return 1;
}
Int2 _windowSize(UWPContext *self) {
   //return glWindowGetSize(self->window);
   return (Int2){ 0 };
}
Microseconds _time(UWPContext *self) {
   Microseconds out = 0;
   //QueryPerformanceCounter((LARGE_INTEGER*)&out);
   out -= self->startTime;
   return (out * 1000000) / self->clockFreq;

}
void _destroy(UWPContext *self) {
   //glWindowDestroy(self->window);
   checkedFree(self);
}
Keyboard *_keyboard(UWPContext *self) {
   //return glWindowGetKeyboard(self->window);
   return 0;
}
Mouse* _mouse(UWPContext *self) {
   //return glWindowGetMouse(self->window);
   return 0;
}