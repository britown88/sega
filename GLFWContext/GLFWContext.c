#include "GLFWContext.h"

#include "GLWindow.h"
#include "SEGA\IDeviceContext.h"
#include "SEGA\App.h"

#include "GLFW/glfw3.h"
#include "segashared/CheckedMemory.h"
#include "SEGA\Input.h"

#include <stdio.h>

#define ClosureTPart \
    CLOSURE_RET(Int2) \
    CLOSURE_NAME(MousePos) \
    CLOSURE_ARGS()
#include "segautils\Closure_Impl.h"

typedef struct {
   IDeviceContext context;
   GLWindow *window;
}GLFWContext;

static int _init(GLFWContext *self, int width, int height, StringView winTitle, int flags);
static void _preRender(GLFWContext *self);
static void _postRender(GLFWContext *self);
static int _shouldClose(GLFWContext *self);
static Int2 _windowSize(GLFWContext *self);
static double _time(GLFWContext *self);
static void _destroy(GLFWContext *self);
static Keyboard* _keyboard(GLFWContext *self);
static Mouse* _mouse(GLFWContext *self);

static IDeviceContextVTable *_getTable(){
   static IDeviceContextVTable *out = NULL;
   if (!out){
      out = calloc(1, sizeof(IDeviceContextVTable));
      out->init = (int(*)(IDeviceContext*, int, int, StringView, int))&_init;
      out->preRender = (void(*)(IDeviceContext*))&_preRender;
      out->postRender = (void(*)(IDeviceContext*))&_postRender;
      out->shouldClose = (int(*)(IDeviceContext*))&_shouldClose;
      out->windowSize = (Int2(*)(IDeviceContext*))&_windowSize;
      out->time = (double(*)(IDeviceContext*))&_time;
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
Int2 _windowSize(GLFWContext *self){
   return glWindowGetSize(self->window);
}
double _time(GLFWContext *self){
   return glfwGetTime();
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

