#include "segautils/IncludeWindows.h"

#include <strsafe.h>


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

static void _renderUpdateFrameTime(Frame *frame, Microseconds frameLength) {
   static byte frameTimes[EGA_RES_WIDTH] = { 0 };
   static int index = 0;
   int i;

   frameTimes[index++] = (byte)t_u2m(frameLength);
   if (index > EGA_RES_WIDTH) {
      index = 0;
   }

   for (i = 0; i < EGA_RES_WIDTH; ++i) {
      byte color = 10;
      if (i == index) {
         color = 0;
      }
      else if (i == index - 1) {
         color = 2;
      }

      frameRenderLine(frame, FrameRegionFULL,
         i, EGA_RES_HEIGHT - 16 - MAX(0, frameTimes[i] - 10),
         i, EGA_RES_HEIGHT - 16, color);
   }


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

static void _singleUpdate(App *self, Microseconds frameLength) {

   virtualAppOnStep(self->subclass);
   iDeviceContextPreRender(self->context);
   //_renderUpdateFrameTime(self->subclass->currentFrame, frameLength);
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
      _singleUpdate(self, deltaTime);
      
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
int appLoadPalette(App *self, const char *palFile){
   Palette pal = paletteDeserialize(palFile);

   if (!memcmp(&pal, &(Palette){0}, sizeof(Palette))) {
      return 1;
   }

   paletteCopy(&self->subclass->currentPalette, &pal);

   return 0;
}
void appSetPalette(App *self, Palette *p){
   paletteCopy(&self->subclass->currentPalette, p);
}

int appListFiles(App *self, const char *root, int type, vec(StringPtr) **out, const char *ext) {
   WIN32_FIND_DATA ffd;
   TCHAR szDir[MAX_PATH];
   size_t length_of_arg;
   HANDLE hFind = INVALID_HANDLE_VALUE;
   DWORD dwError = 0;
   int extLen = 0;

   // Check that the input path plus 3 is not longer than MAX_PATH.
   // Three characters are for the "\*" plus NULL appended below.

   StringCchLength(root, MAX_PATH, &length_of_arg);

   if (length_of_arg > (MAX_PATH - 3)) {
      //_tprintf(TEXT("\nDirectory path is too long.\n"));
      return 1;
   }

   //_tprintf(TEXT("\nTarget directory is %s\n\n"), argv[1]);

   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.

   StringCchCopy(szDir, MAX_PATH, root);
   StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

   // Find the first file in the directory.

   hFind = FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind)   {
      //DisplayErrorBox(TEXT("FindFirstFile"));
      return dwError;
   }

   *out = vecCreate(StringPtr)(&stringPtrDestroy);

   if (ext) {
      extLen = strlen(ext);
   }

   // List all the files in the directory with some info about them.
   do {

      if (type == APP_FILE_ALL ||
         (type == APP_FILE_DIR_ONLY && ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
         (type == APP_FILE_FILE_ONLY && !(ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))) {

         String *fname = stringCreate(root);
         stringConcat(fname, "/");
         stringConcat(fname, ffd.cFileName);

         if (ext) {
            int len = strlen(ffd.cFileName);
            size_t dotPos;
            char *str;

            if (len <= extLen + 1) {
               stringDestroy(fname);
               continue;
            }

            dotPos = stringFindLastOf(fname, ".");
            str = c_str(fname) + dotPos + 1;

            if (dotPos < stringNPos && 
               strlen(str) == extLen &&               
               !memcmp(str, ext, extLen)) {

               vecPushBack(StringPtr)(*out, &fname);
            }
            else {
               stringDestroy(fname);
            }
         }
         else {
            vecPushBack(StringPtr)(*out, &fname);
         }
      }

      
   } while (FindNextFile(hFind, &ffd) != 0);


   FindClose(hFind);
   return 0;
}