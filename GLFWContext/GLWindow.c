#include "GLFW/glfw3.h"
#include "GLWindow.h"
#include "segashared\CheckedMemory.h"
#include "SEGA\Input.h"

#include <malloc.h>
#include <stddef.h>
#include <Windows.h>

struct GLWindow_T {
   GLFWwindow* window;
   Int2 winSize;
   Keyboard *keyboard;
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

   r = checkedCalloc(1, sizeof(GLWindow));
   r->window = window;
   r->winSize = actualSize;   
   r->keyboard = keyboardCreate();

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
Float2 glWindowGetMousePos(GLWindow *self){
   double x, y;
   glfwGetCursorPos(self->window, &x, &y);
   return float2Create((float)x, (float)y);
}
Keyboard *glWindowGetKeyboard(GLWindow *self){
   return self->keyboard;
}
