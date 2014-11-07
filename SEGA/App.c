#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include <malloc.h>
#include <stddef.h>

#include "Defs.h"
#include "App.h"
#include "GLWindow.h"
#include "Renderer.h"
#include "EGADisplay.h"
#include "EGAPalette.h"
#include "segalib\CheckedMemory.h"
#include "FBO.h"
#include "PaletteTable.h"

struct App_t {
   VirtualApp *subclass;
   bool running;
   double frameRate;
   double lastUpdated;

   GLWindow *window;
   Renderer *renderer;
   EGADisplay *egaDisplay;
   FBO *egaFrameBuffer;
   PaletteTable *pTable;
   Rectf viewport;
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

static Rectf egaBounds = {0.0f, 0.0f, (float)EGA_RES_WIDTH, (float)EGA_RES_HEIGHT};

static void _preRender(App *self) {
   byte *p = self->subclass->currentPalette.colors;
   EGAPalette *ep = paletteTableGetPalette(self->pTable, p);

   egaDisplaySetPalette(self->egaDisplay, ep);

   egaDisplayRenderFrame(self->egaDisplay, self->subclass->currentFrame);

}

static void _render(App *self) {
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   //render to ogl fbo
   fboBind(self->egaFrameBuffer);
   rendererPushViewport(self->renderer, egaBounds);
   egaDisplayRender(self->egaDisplay, self->renderer);
   rendererPopViewport(self->renderer); 

   //render to screen
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   rendererPushViewport(self->renderer, self->viewport);
   fboRender(self->egaFrameBuffer, self->renderer);
   rendererPopViewport(self->renderer);

   //swap
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

      _preRender(self);
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

   r->egaDisplay = egaDisplayCreate();
   r->viewport = _buildProportionalViewport(data.defaultWindowSize.x, data.defaultWindowSize.y);
   r->egaFrameBuffer = fboCreate(EGA_RES_WIDTH, EGA_RES_HEIGHT);
   r->pTable = paletteTableCreate();

   r->subclass->currentFrame = frameCreate();
   memset(&r->subclass->currentPalette.colors, 0, EGA_PALETTE_COLORS);

   virtualAppOnStart(r->subclass);

   r->running = true;

   while(r->running) {
      _step(r);
   }

   paletteTableDestroy(r->pTable);
   egaDisplayDestroy(r->egaDisplay);
   fboDestroy(r->egaFrameBuffer);
   rendererDestroy(r->renderer);

   glWindowDestroy(r->window);
   virtualAppDestroy(r->subclass);
   checkedFree(r);
   return;
}

double appGetTime(App *self){return glfwGetTime() * 1000;}
double appGetFrameTime(App *self){return 1000.0 / self->frameRate;}
double appGetFrameRate(App *self){return self->frameRate;}