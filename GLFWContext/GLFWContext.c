#include "GLFWContext.h"

#include "GLWindow.h"
#include "SEGA\IDeviceContext.h"
#include "SEGA\App.h"

#include "segautils/IncludeWindows.h"
#include "GLFW/glfw3.h"
#include "segashared/CheckedMemory.h"
#include "SEGA\Input.h"

#include <stdio.h>

#define ClosureTPart CLOSURE_NAME(MousePos)
#include "segautils\Closure_Impl.h"

typedef struct {
   IDeviceContext context;
   GLWindow *window;   
   
   uint64_t clockFreq;
   Microseconds startTime;
}GLFWContext;

static int _init(GLFWContext *self, int width, int height, StringView winTitle, int flags);
static void _initRendering(GLFWContext *self);
static void _commitRender(GLFWContext *self);
static void _preRender(GLFWContext *self);
static void _postRender(GLFWContext *self);
static int _shouldClose(GLFWContext *self);
static Int2 _windowSize(GLFWContext *self);
static Microseconds _time(GLFWContext *self);
static void _destroy(GLFWContext *self);
static Keyboard* _keyboard(GLFWContext *self);
static Mouse* _mouse(GLFWContext *self);

static IDeviceContextVTable *_getTable(){
   static IDeviceContextVTable *out = NULL;
   if (!out){
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

IDeviceContext *createGLFWContext(){
   GLFWContext *out = checkedCalloc(1, sizeof(GLFWContext));
   out->context.vTable = _getTable();
   return (IDeviceContext *)out;
}

int _init(GLFWContext *self, int width, int height, StringView winTitle, int flags){
   Int2 size = { width, height };
   GLFWmonitor *monitor = NULL;

   if (!glfwInit()){
      return 1;
   }

   if (flags&DC_FLAG_FULLSCREEN){
      monitor = glfwGetPrimaryMonitor();
   }


   self->window = glWindowCreate(size, winTitle, monitor);

   if (!self->window){
      glfwTerminate();
      return 1;
   }

   QueryPerformanceFrequency((LARGE_INTEGER*)&self->clockFreq);
   QueryPerformanceCounter((LARGE_INTEGER*)&self->startTime);
   return 0;
}

void _initRendering(GLFWContext *self) {
   glWindowBeginRendering(self->window);
}
void _commitRender(GLFWContext *self) {
   glWindowSwapBuffers(self->window);
}
void _preRender(GLFWContext *self){

}
void _postRender(GLFWContext *self){   
   glWindowPollEvents(self->window);
}
int _shouldClose(GLFWContext *self){
   return glWindowShouldClose(self->window);
}
Int2 _windowSize(GLFWContext *self){
   return glWindowGetSize(self->window);
}
Microseconds _time(GLFWContext *self){
   Microseconds out;
   QueryPerformanceCounter((LARGE_INTEGER*)&out);
   out -= self->startTime;
   return (out * 1000000) / self->clockFreq;

}
void _destroy(GLFWContext *self){
   glWindowDestroy(self->window);
   checkedFree(self);
}
Keyboard *_keyboard(GLFWContext *self){
   return glWindowGetKeyboard(self->window);
}
Mouse* _mouse(GLFWContext *self){
   return glWindowGetMouse(self->window);
}

