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
}TestGLContext;

static int _init(TestGLContext *self, int width, int height, StringView winTitle, int flags);
static void _preRender(TestGLContext *self);
static void _postRender(TestGLContext *self);
static int _shouldClose(TestGLContext *self);
static Int2 _windowSize(TestGLContext *self);
static double _time(TestGLContext *self);
static void _destroy(TestGLContext *self);

static IDeviceContextVTable *_getTable(){
   static IDeviceContextVTable *out = NULL;
   if (!out){
      out = checkedCalloc(1, sizeof(IDeviceContextVTable));
      out->init = (int(*)(IDeviceContext*, int, int, StringView, int))&_init;
      out->preRender = (void(*)(IDeviceContext*))&_preRender;
      out->postRender = (void(*)(IDeviceContext*))&_postRender;
      out->shouldClose = (int(*)(IDeviceContext*))&_shouldClose;
      out->windowSize = (Int2(*)(IDeviceContext*))&_windowSize;
      out->time = (double(*)(IDeviceContext*))&_time;
      out->destroy = (void(*)(IDeviceContext*))&_destroy;
   }
   return out;
}

IDeviceContext *createGLFWContext(){
   TestGLContext *out = checkedCalloc(1, sizeof(TestGLContext));
   out->context.vTable = _getTable();
   return (IDeviceContext *)out;
}

int _init(TestGLContext *self, int width, int height, StringView winTitle, int flags){
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
void _preRender(TestGLContext *self){

}
void _postRender(TestGLContext *self){
   glWindowSwapBuffers(self->window);
   glWindowPollEvents(self->window);
}
int _shouldClose(TestGLContext *self){
   return glWindowShouldClose(self->window);
}
Int2 _windowSize(TestGLContext *self){
   return glWindowGetSize(self->window);
}
double _time(TestGLContext *self){
   return glfwGetTime();
}
void _destroy(TestGLContext *self){
   glWindowDestroy(self->window);
   checkedFree(self);
}

