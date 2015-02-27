#include "GLFW/glfw3.h"
#include "GLWindow.h"
#include "segashared\CheckedMemory.h"
#include "IDeviceContext.h"
#include "App.h"

#include <malloc.h>
#include <stddef.h>

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

IDeviceContext *testGLContext(){
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










struct GLWindow_T {
   GLFWwindow* window;
   Int2 winSize;
};

GLWindow *glWindowCreate(Int2 winSize, StringView windowName, GLFWmonitor *monitor){
   GLFWwindow *window;
   Int2 actualSize;
   GLWindow *r;
   
   if(!monitor) {
      glfwWindowHint(GLFW_RESIZABLE, 0);
   }

   //glfwWindowHint(GLFW_DECORATED, 0);   

   window = glfwCreateWindow(winSize.x, winSize.y, windowName, monitor, NULL);

   if(!window) {
      glfwTerminate();
      return NULL;
   }
   
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
   glfwGetFramebufferSize(window, &actualSize.x, &actualSize.y);
   glfwMakeContextCurrent(window);

   r = checkedCalloc(1, sizeof(GLWindow));
   r->window = window;
   r->winSize = actualSize;   

   return r;
}
void glWindowDestroy(GLWindow *self){
   glfwDestroyWindow(self->window);
   glfwTerminate();
   checkedFree(self);
}

void glWindowPollEvents(GLWindow *self){
   glfwPollEvents();
}
void glWindowSwapBuffers(GLWindow *self){
   glfwSwapBuffers(self->window);
}
int glWindowShouldClose(GLWindow *self){
   return glfwWindowShouldClose(self->window);
}
Int2 glWindowGetSize(GLWindow *self){
   return self->winSize;
}
