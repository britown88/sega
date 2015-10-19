#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


#include <malloc.h>
#include <stddef.h>
#include <time.h>

#include "segautils\Defs.h"
#include "App.h"
#include "segautils\Rect.h"

#include "segashared\CheckedMemory.h"
#include "IDeviceContext.h"
#include "Input.h"

App *g_App;

App *appGet(){
   return g_App;
}


struct App_t {
   VirtualApp *subclass;
   bool running;
   double desiredFrameRate;
   Microseconds lastUpdated;
   float vpScale;
 
   Int2 winSize;
   Rectf viewport;
   IRenderer *renderer;
   IDeviceContext *context;

};

void appSleep(int ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  usleep(ms*1000);
#endif
}

Rectf _buildProportionalViewport(int width, int height, float *ratio)
{
   float rw = (float)width;
   float rh = (float)height;
   float cw = EGA_RES_WIDTH * EGA_PIXEL_WIDTH;
   float ch = EGA_RES_HEIGHT * EGA_PIXEL_HEIGHT;
   *ratio = MIN(rw/cw, rh/ch);

   Rectf vp = {0.0f, 0.0f, cw * (*ratio), ch * (*ratio)};
   rectfOffset(&vp, (rw - rectfWidth(&vp)) / 2.0f, (rh - rectfHeight(&vp)) / 2.0f);

   return vp;
}

static void _updateFPS(Microseconds delta, double *fps){
   static Microseconds time = 0;
   static const Microseconds interval = 500000;
   static double sample = 0.0;
   static int sampleCount = 0;

   time += delta;

   sample += 1000000.0 / (delta);
   ++sampleCount;

   if (time > interval){      
      time -= interval;
      *fps = sample / sampleCount;
      sample = 0.0;
      sampleCount = 0;
   }

}

static void _singleUpdate(App *self) {

   virtualAppOnStep(self->subclass);
   iDeviceContextPreRender(self->context);
   iRendererRenderFrame(self->renderer,
      self->subclass->currentFrame,
      self->subclass->currentPalette.colors,
      &self->viewport);
   iDeviceContextPostRender(self->context);

   if (iDeviceContextShouldClose(self->context))
      self->running = false;
}

static void _step(App *self) {
   //dt
   Microseconds usPerFrame = appGetFrameTime(self);
   Microseconds time = appGetTime(self);
   Microseconds deltaTime = time - self->lastUpdated;   

   //update
   if(deltaTime >= usPerFrame)
   {      
      self->lastUpdated = time;      
      _updateFPS(deltaTime, &virtualAppGetData(self->subclass)->fps);
      _singleUpdate(self);
   }
   else if(usPerFrame - deltaTime > 3000){
      //only yield if we're more than 3ms out
      appSleep(0);
   }
}

App *_createApp(VirtualApp *subclass, IDeviceContext *context, IRenderer *renderer){
   AppData *data = virtualAppGetData(subclass);
   App *out = checkedCalloc(1, sizeof(App));
   out->winSize = iDeviceContextWindowSize(context);
   out->subclass = subclass;

   out->lastUpdated = 0;
   out->desiredFrameRate = data->desiredFrameRate;

   out->renderer = renderer;
   out->context = context;

   out->viewport = _buildProportionalViewport(out->winSize.x, out->winSize.y, &out->vpScale);

   out->running = true;
   g_App = out;

   subclass->currentFrame = frameCreate();
   iRendererInit(renderer);
   virtualAppOnStart(subclass);

   

   return out;
}

void _destroyApp(App *self){
   iRendererDestroy(self->renderer);
   iDeviceContextDestroy(self->context);
   frameDestroy(self->subclass->currentFrame);
   virtualAppDestroy(self->subclass);
   checkedFree(self);
}

void runApp(VirtualApp *subclass, IRenderer *renderer, IDeviceContext *context) {
   AppData *data = virtualAppGetData(subclass);
   Int2 winSize = data->defaultWindowSize;
   App *app;
   int result = iDeviceContextInit(context, winSize.x, winSize.y, data->windowTitle, data->dcFlags);

   if (result){
      return;
   }

   srand((unsigned int)time(NULL));

   //update winSize
   winSize = iDeviceContextWindowSize(context);

   app = _createApp(subclass, context, renderer);

   while (app->running) {
      _step(app);
   }

   _destroyApp(app);   
   
   return;
}

//inclusive lower exclusive upper
int appRand(App *self, int lower, int upper){
   return (rand() % (upper - lower)) + lower;
}
Int2 appWindowToWorld(App *self, Float2 coords){
   Int2 out = { 0, 0 };

   //now convert the damn coords to vp
   coords.x -= self->viewport.left;
   coords.y -= self->viewport.top;

   out.x = (int)((coords.x / rectfWidth(&self->viewport)) * (float)EGA_RES_WIDTH);
   out.y = (int)((coords.y / rectfHeight(&self->viewport)) * (float)EGA_RES_HEIGHT);

   return out;
}

Keyboard *appGetKeyboard(App *self){
   return iDeviceContextKeyboard(self->context);
}

Mouse *appGetMouse(App *self){
   return iDeviceContextMouse(self->context);
}

Microseconds appGetTime(App *self){return iDeviceContextTime(self->context);}
Microseconds appGetFrameTime(App *self){ 
   static Microseconds out;
   static bool outSet = false;
   if (!outSet) {
      out = t_s2u(1.0 / self->desiredFrameRate);
   }
   return out; 
}
double appGetFrameRate(App *self){return self->desiredFrameRate;}

void appQuit(App *app){
   app->running = false;
}

Palette *appGetPalette(App *self){
   return &self->subclass->currentPalette;
}
void appLoadPalette(App *self, const char *palFile){
   Palette pal = paletteDeserialize(palFile);
   paletteCopy(&self->subclass->currentPalette, &pal);
}
void appSetPalette(App *self, Palette *p){
   paletteCopy(&self->subclass->currentPalette, p);
}