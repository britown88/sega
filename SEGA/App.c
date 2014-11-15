#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


#include <malloc.h>
#include <stddef.h>

#include "segautils\Defs.h"
#include "App.h"
#include "GLWindow.h"
#include "segautils\Rect.h"

#include "segashared\CheckedMemory.h"
#include "GLFW/glfw3.h"


struct App_t {
   VirtualApp *subclass;
   bool running;
   double frameRate;
   double lastUpdated;

   GLWindow *window;   
   Rectf viewport;
   IRenderer *renderer;
};

App *g_app;

void appSleep(int ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  usleep(ms*1000);
#endif
}

Rectf _buildProportionalViewport(int width, int height)
{
   float rw = (float)width;
   float rh = (float)height;
   float cw = EGA_RES_WIDTH * EGA_PIXEL_WIDTH;
   float ch = EGA_RES_HEIGHT * EGA_PIXEL_HEIGHT;
   float ratio = MIN(rw/cw, rh/ch);

   Rectf vp = {0.0f, 0.0f, cw * ratio, ch * ratio};
   rectfOffset(&vp, (rw - rectfWidth(&vp)) / 2.0f, (rh - rectfHeight(&vp)) / 2.0f);

   return vp;
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

      iRendererRenderFrame(self->renderer,
         self->subclass->currentFrame,
         self->subclass->currentPalette.colors,
         &self->viewport);

      //swap
      glWindowSwapBuffers(self->window);
      
      glWindowPollEvents(self->window);

      if(glWindowShouldClose(self->window))
         self->running = false;  
   }
   else
      appSleep(0);
}

void runApp(VirtualApp *subclass, IRenderer *renderer) {
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
   r->viewport = _buildProportionalViewport(data.defaultWindowSize.x, data.defaultWindowSize.y);

   r->renderer = renderer;
   iRendererInit(r->renderer);
   

   r->subclass->currentFrame = frameCreate();
   memset(&r->subclass->currentPalette.colors, 0, EGA_PALETTE_COLORS);

   virtualAppOnStart(r->subclass);

   r->running = true;

   while(r->running) {
      _step(r);
   }

   
   iRendererDestroy(r->renderer);
   glWindowDestroy(r->window);
	frameDestroy(r->subclass->currentFrame);
   virtualAppDestroy(r->subclass);
   checkedFree(r);
   return;
}

double appGetTime(App *self){return glfwGetTime() * 1000;}
double appGetFrameTime(App *self){return 1000.0 / self->frameRate;}
double appGetFrameRate(App *self){return self->frameRate;}