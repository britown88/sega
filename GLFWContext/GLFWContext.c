#include "GLFWContext.h"

#include "GLWindow.h"
#include "SEGA\IDeviceContext.h"
#include "SEGA\App.h"

#include "GLFW/glfw3.h"
#include "segashared/CheckedMemory.h"

#include <stdio.h>

typedef struct {
   IDeviceContext context;
   GLWindow *window;
}GLFWContext;

static int _init(GLFWContext *self, int width, int height, StringView winTitle, int flags);
static void _preRender(GLFWContext *self);
static void _postRender(GLFWContext *self);
static int _shouldClose(GLFWContext *self);
static int _pointerEnabled(GLFWContext *self);
static Int2 _windowSize(GLFWContext *self);
static Float2 _pointerPos(GLFWContext *self);
static double _time(GLFWContext *self);
static void _destroy(GLFWContext *self);

static IDeviceContextVTable *_getTable(){
   static IDeviceContextVTable *out = NULL;
   if (!out){
      out = calloc(1, sizeof(IDeviceContextVTable));
      out->init = (int(*)(IDeviceContext*, int, int, StringView, int))&_init;
      out->preRender = (void(*)(IDeviceContext*))&_preRender;
      out->postRender = (void(*)(IDeviceContext*))&_postRender;
      out->shouldClose = (int(*)(IDeviceContext*))&_shouldClose;
      out->pointerEnabled = (int(*)(IDeviceContext*))&_pointerEnabled;
      out->windowSize = (Int2(*)(IDeviceContext*))&_windowSize;
      out->pointerPos = (Float2(*)(IDeviceContext*))&_pointerPos;
      out->time = (double(*)(IDeviceContext*))&_time;
      out->destroy = (void(*)(IDeviceContext*))&_destroy;
   }
   return out;
}

IDeviceContext *createGLFWContext(){
   GLFWContext *out = checkedCalloc(1, sizeof(GLFWContext));
   out->context.vTable = _getTable();
   return (IDeviceContext *)out;
}

int _init(GLFWContext *self, int width, int height, StringView winTitle, int flags){
   Int2 size = int2Create(width, height);
   GLFWmonitor *monitor = NULL;

   if (!glfwInit()){
      return 1;
   }

   if (flags&DC_FLAG_FULLSCREEN){
      monitor = glfwGetPrimaryMonitor();
   }

   self->window = glWindowCreate(size, winTitle, monitor);

   if (!self->window){
      return 1;
   }

   return 0;
}
void _preRender(GLFWContext *self){

}
void _postRender(GLFWContext *self){
   glWindowSwapBuffers(self->window);
   glWindowPollEvents(self->window);
}
int _shouldClose(GLFWContext *self){
   return glWindowShouldClose(self->window);
}
int _pointerEnabled(GLFWContext *self){
   return true;
}
Int2 _windowSize(GLFWContext *self){
   return glWindowGetSize(self->window);
}
Float2 _pointerPos(GLFWContext *self){
   return glWindowGetMousePos(self->window);
}
double _time(GLFWContext *self){
   return glfwGetTime();
}
void _destroy(GLFWContext *self){
   glWindowDestroy(self->window);
   checkedFree(self);
}

