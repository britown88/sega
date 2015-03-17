#include <Windows.h>
#include "GLFW/glfw3.h"
#include "GLWindow.h"
#include "segashared\CheckedMemory.h"
#include "SEGA\Input.h"
#include "SEGA\App.h"

#include <malloc.h>
#include <stddef.h>


struct GLWindow_T {
   GLFWwindow* window;
   Int2 winSize;
   Keyboard *keyboard;
   Mouse *mouse;
};

static GLWindow *WinList[8] = { 0 };
static int WinCount = 0;

static void registerNewWindow(GLWindow *win){
   WinList[WinCount++] = win;
}

static GLWindow *getGLWindow(GLFWwindow *glfw){
   int i = 0;
   for (i = 0; i < WinCount; ++i){
      if (WinList[i]->window == glfw){
         return WinList[i];
      }
   }

   return NULL;
}

static void glWindowKeyFunc(GLFWwindow* win, int key, int scancode, int action, int mod){
   GLWindow *self = getGLWindow(win);

   KeyboardEvent e = { 
      .action = getSegaAction(action), 
      .key = getSegaKey(key) 
   };

   keyboardPushEvent(self->keyboard, &e);
}

static Int2 _getMousePos(GLFWwindow *win){
   double x, y;
   glfwGetCursorPos(win, &x, &y);
   return appWindowToWorld(appGet(), (Float2){ (float)x, (float)y });
}

static void glWindowMouseButtonFunc(GLFWwindow *win, int button, int action, int mod){
   GLWindow *self = getGLWindow(win);

   MouseEvent e = {
      .action = action == GLFW_PRESS ? SegaMouse_Pressed : SegaMouse_Released,
      .button = getSegaMouseButton(button),
      .pos = _getMousePos(win)
   };

   mousePushEvent(self->mouse, &e);
}

static void glWindowMouseScrollFunc(GLFWwindow *win, double x, double y){
   GLWindow *self = getGLWindow(win);

   MouseEvent e = {
      .action = SegaMouse_Scrolled,
      .button = 0,
      {.x = (int)x, .y = (int)y}
   };

   mousePushEvent(self->mouse, &e);
}

static void glWindowMouseMoveFunc(GLFWwindow *win, double x, double y){
   GLWindow *self = getGLWindow(win);

   MouseEvent e = {
      .action = SegaMouse_Moved,
      .button = 0,
      .pos = appWindowToWorld(appGet(), (Float2){ (float)x, (float)y })
   };

   mousePushEvent(self->mouse, &e);
}



static void _glWindowInitMouseClosure(GLWindow *self){
   MousePos getPos;
   closureInit(MousePos)(&getPos, self->window, (MousePosFunc)&_getMousePos, NULL);
   self->mouse = mouseCreate(getPos);
}

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

   FreeConsole();
   
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
   glfwGetFramebufferSize(window, &actualSize.x, &actualSize.y);
   glfwMakeContextCurrent(window);
   glfwSetKeyCallback(window, &glWindowKeyFunc);
   glfwSetMouseButtonCallback(window, &glWindowMouseButtonFunc);
   glfwSetCursorPosCallback(window, &glWindowMouseMoveFunc);
   glfwSetScrollCallback(window, &glWindowMouseScrollFunc);

   r = checkedCalloc(1, sizeof(GLWindow));
   r->window = window;
   r->winSize = actualSize;   
   r->keyboard = keyboardCreate();
   _glWindowInitMouseClosure(r);

   registerNewWindow(r);

   return r;
}
void glWindowDestroy(GLWindow *self){
   keyboardDestroy(self->keyboard);
   glfwDestroyWindow(self->window);
   glfwTerminate();
   checkedFree(self);
}

void glWindowPollEvents(GLWindow *self){
   keyboardFlushQueue(self->keyboard);
   mouseFlushQueue(self->mouse);
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
Keyboard *glWindowGetKeyboard(GLWindow *self){
   return self->keyboard;
}
Mouse *glWindowGetMouse(GLWindow *self){
   return self->mouse;
}
