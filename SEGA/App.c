#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <GLFW/glfw3.h>
#include <malloc.h>
#include <stddef.h>

#include "Defs.h"
#include "App.h"
#include "GLWindow.h"
#include "Renderer.h"
#include "segalib\CheckedMemory.h"

struct App_t {
   VirtualApp *subclass;
   bool running;
   double frameRate;
   double lastUpdated;

   GLWindow *window;
   Renderer *renderer;
};

App *g_app;

void appSleep(int ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  usleep(ms*1000);
#endif
}

static void _render(App *self) {
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   virtualAppOnRender(self->subclass, self->renderer);

   glWindowSwapBuffers(self->window);
}

static void _step(App *self) {
   //dt
   double time = appGetTime(self);
   double deltaTime = time - self->lastUpdated;
   double dt = deltaTime / appGetFrameTime(self);

   //update
   if(dt >= 1.0)
   {
      self->lastUpdated = time;

      virtualAppOnStep(self->subclass);

      _render(self);
      
      
      glWindowPollEvents(self->window);

      if(glWindowShouldClose(self->window))
         self->running = false;  
   }
   else
      appSleep(0);
}

void runApp(VirtualApp *subclass) {
   AppData data;
   GLWindow *window;
   App *r;

   if (!glfwInit()){
      return;
   }

   data = virtualAppGetData(subclass);

   window = glWindowCreate(data.defaultWindowSize, data.windowTitle, data.fullScreen ? glfwGetPrimaryMonitor() : NULL);
   if(!window) {
      virtualAppDestroy(subclass);
      return;
   }

   r = checkedCalloc(1, sizeof(App));
   r->subclass = subclass;
   g_app = r;

   r->lastUpdated = 0.0;
   r->frameRate = data.frameRate;

   r->window = window;
   r->renderer = rendererCreate();

   virtualAppOnStart(r->subclass);

   r->running = true;

   while(r->running) {
      _step(r);
   }

   rendererDestroy(r->renderer);

   glWindowDestroy(r->window);
   virtualAppDestroy(r->subclass);
   checkedFree(r);
   return;
}

double appGetTime(App *self){return glfwGetTime() * 1000;}
double appGetFrameTime(App *self){return 1000.0 / self->frameRate;}
double appGetFrameRate(App *self){return self->frameRate;}