#include "pch.h"
#include "DeviceContext.h"

#include "SEGA\IDeviceContext.h"
#include "SEGA\App.h"

#include "segashared/CheckedMemory.h"
#include "SEGA\Input.h"


#define ClosureTPart CLOSURE_NAME(MousePos)
#include "segautils\Closure_Impl.h"


#include <malloc.h>
#include <stddef.h>

struct UWPWindow {
   Keyboard *keyboard;
   Mouse *mouse;
   UWP::UWPMain *main;
};


static Int2 _getMousePos() {
   Int2 pos = { 0 };
   return pos;
}

//static void glWindowMouseButtonFunc(GLFWwindow *win, int button, int action, int mod) {
//   GLWindow *self = getGLWindow(win);
//
//   MouseEvent e = {
//      .action = action == GLFW_PRESS ? SegaMouse_Pressed : SegaMouse_Released,
//      .button = getSegaMouseButton(button),
//      .pos = _getMousePos(win)
//   };
//
//   mousePushEvent(self->mouse, &e);
//}
//
//static void glWindowMouseScrollFunc(GLFWwindow *win, double x, double y) {
//   GLWindow *self = getGLWindow(win);
//
//   MouseEvent e = {
//      .action = SegaMouse_Scrolled,
//      .button = 0,
//      { .x = (int)x,.y = (int)y }
//   };
//
//   mousePushEvent(self->mouse, &e);
//}
//
//static void glWindowMouseMoveFunc(GLFWwindow *win, double x, double y) {
//   GLWindow *self = getGLWindow(win);
//
//   MouseEvent e = {
//      .action = SegaMouse_Moved,
//      .button = 0,
//      .pos = appWindowToWorld(appGet(), (Float2) { (float)x, (float)y })
//   };
//
//   mousePushEvent(self->mouse, &e);
//}



static void _uwpWindowInitMouseClosure(UWPWindow *self) {
   MousePos getPos;
   closureInit(MousePos)(&getPos, NULL, (MousePosFunc)&_getMousePos, NULL);
   self->mouse = mouseCreate(getPos);
}

UWPWindow *uwpWindowCreate(Int2 winSize, StringView windowName) {
   Int2 actualSize;
   UWPWindow *r;

   //FreeConsole();

   //glfwSetInputMode(threadWin, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
   //glfwGetFramebufferSize(threadWin, &actualSize.x, &actualSize.y);
   //glfwMakeContextCurrent(window);
   //glfwSetKeyCallback(threadWin, &glWindowKeyFunc);
   //glfwSetCharCallback(threadWin, &glWindowCharFun);
   //glfwSetMouseButtonCallback(threadWin, &glWindowMouseButtonFunc);
   //glfwSetCursorPosCallback(threadWin, &glWindowMouseMoveFunc);
   //glfwSetScrollCallback(threadWin, &glWindowMouseScrollFunc);

   r = (UWPWindow*)checkedCalloc(1, sizeof(UWPWindow));

   r->keyboard = keyboardCreate();
   _uwpWindowInitMouseClosure(r);

   //registerNewWindow(r);

   return r;
}

void uwpWindowBeginRendering(UWPWindow *self) {
   //glfwMakeContextCurrent(self->threadWin);
   //glfwSwapInterval(1);
}

void uwpWindowDestroy(UWPWindow *self) {
   keyboardDestroy(self->keyboard);
   mouseDestroy(self->mouse);
   //glfwDestroyWindow(self->window);
   //glfwDestroyWindow(self->threadWin);
   //glfwTerminate();
   checkedFree(self);
}

void uwpWindowPollEvents(UWPWindow *self) {
   keyboardFlushQueue(self->keyboard);
   mouseFlushQueue(self->mouse);
   //glfwPollEvents();
}
void uwpWindowSwapBuffers(UWPWindow *self) {
   //glfwSwapBuffers(self->threadWin);
}
int uwpWindowShouldClose(UWPWindow *self) {
   return 0;
}
Int2 uwpWindowGetSize(UWPWindow *self) {
   auto sz = self->main->getOutputSize();
   Int2 out = { 0 };
   out.x = sz.Width;
   out.y = sz.Height;
   return out;
}
Keyboard *uwpWindowGetKeyboard(UWPWindow *self) {
   return self->keyboard;
}
Mouse *uwpWindowGetMouse(UWPWindow *self) {
   return self->mouse;
}

typedef struct  {
   IDeviceContext context;

   uint64_t clockFreq;
   Microseconds startTime;
   UWPWindow *window;
   UWP::UWPMain *main;
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
      out = (IDeviceContextVTable*)calloc(1, sizeof(IDeviceContextVTable));
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

IDeviceContext *createUWPContext(UWP::UWPMain *main) {
   UWPContext *out = (UWPContext*)checkedCalloc(1, sizeof(UWPContext));
   out->context.vTable = _getTable();
   out->main = main;
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


   self->window = uwpWindowCreate(size, winTitle);
   self->window->main = self->main;

   //if (!self->window) {
   //   glfwTerminate();
   //   return 1;
   //}

   QueryPerformanceFrequency((LARGE_INTEGER*)&self->clockFreq);
   QueryPerformanceCounter((LARGE_INTEGER*)&self->startTime);
   return 0;
}

void _initRendering(UWPContext *self) {
   uwpWindowBeginRendering(self->window);
}
void _commitRender(UWPContext *self) {
   uwpWindowSwapBuffers(self->window);
}
void _preRender(UWPContext *self) {

}
void _postRender(UWPContext *self) {
   uwpWindowPollEvents(self->window);
}
int _shouldClose(UWPContext *self) {
   return uwpWindowShouldClose(self->window);
}
Int2 _windowSize(UWPContext *self) {

   return uwpWindowGetSize(self->window);
   
}
Microseconds _time(UWPContext *self) {
   Microseconds out = 0;
   QueryPerformanceCounter((LARGE_INTEGER*)&out);
   out -= self->startTime;
   return (out * 1000000) / self->clockFreq;

}
void _destroy(UWPContext *self) {
   uwpWindowDestroy(self->window);
   checkedFree(self);
}
Keyboard *_keyboard(UWPContext *self) {
   return uwpWindowGetKeyboard(self->window);
}
Mouse* _mouse(UWPContext *self) {
   return uwpWindowGetMouse(self->window);
   return 0;
}