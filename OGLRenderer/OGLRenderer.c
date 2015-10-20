#include "defined_gl.h"
#include "OGLRenderer.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "SEGA/App.h"

#include "EGATexture.h"

#include "segautils/IncludeWindows.h"
DWORD WINAPI _renderThread(LPVOID lpParam);

typedef struct ThreadData_t ThreadData;

typedef struct {
   IRenderer ir;
   EGATexture *tex;
   IDeviceContext *context;
   ThreadData *thread;

} OGLRenderer;

static void _Init(OGLRenderer*);
static void _RenderFrame(OGLRenderer*, Frame *, byte *, Rectf *);
static void _Destroy(OGLRenderer*);

static IRendererVTable *_getTable() {
   static IRendererVTable *r = 0;
   if (!r){
      r = calloc(1, sizeof(IRendererVTable));
      r->init = (void(*)(IRenderer*))&_Init;
      r->renderFrame = (void(*)(IRenderer*, Frame*, byte*, Rectf *))&_RenderFrame;
      r->destroy = (void(*)(IRenderer*))&_Destroy;
   }

   return r;
}

IRenderer *createOGLRenderer(IDeviceContext *context){
	OGLRenderer *r = checkedCalloc(1, sizeof(OGLRenderer));
   r->ir.vTable = _getTable();
   r->context = context;

   return (IRenderer *)r;
}

void _performRender(OGLRenderer *self, Rectf *vp) {
   glViewport((int)vp->left, (int)vp->top, (int)rectfWidth(vp), (int)rectfHeight(vp));
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, EGA_RES_WIDTH, EGA_RES_HEIGHT, 0, 1.f, -1.f);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   egaTextureRenderFrame(self->tex);
}

typedef struct {
   Frame frame;
   Palette palette;
   Rectf vp;
}Scene;

typedef struct ThreadData_t {
   bool running;
   OGLRenderer *renderer;
   HANDLE handle, mutex;

   Scene scenes[2];
   Scene *front, *back;
   int frame;
}ThreadData;

void _startThread(OGLRenderer *self) {
   ThreadData *data = checkedCalloc(1, sizeof(ThreadData));
   DWORD threadID;

   data->front = data->scenes;
   data->back = data->scenes + 1;

   data->running = true;
   data->renderer = self;

   data->mutex = CreateMutex(NULL, FALSE, NULL);
   data->handle = CreateThread(NULL, 0, _renderThread, data, 0, &threadID);
   self->thread = data;
}

#define NONE 0
#define DUPE (1 << 1)
#define DROP (1 << 2)

static byte frameTimes[EGA_RES_WIDTH] = { 0 };
static byte status[EGA_RES_WIDTH] = { 0 };
static int index = 0;

static void _pushFrameResult(Scene *scene, Microseconds frameLength, int s) {
   status[index] = s;
   frameTimes[index++] = (byte)t_u2m(frameLength);

   if (index > EGA_RES_WIDTH) {
      index = 0;
   }
}

static void _renderFrameTime(Scene *scene) {   
   int i;  

   for (i = 0; i < EGA_RES_WIDTH; ++i) {
      byte color = 15;
      if (i == index) {
         color = 0;
      }
      else if (i == index - 1) {
         color = 4;
      }
      else if (i == index - 2) {
         color = 12;
      }
      else if (status[i]&DROP) {
         color = 3;
      }
      else if (status[i]&DUPE) {
         color = 13;
      }      

      frameRenderLine(&scene->frame, FrameRegionFULL,
         i, EGA_RES_HEIGHT - 14,
         i, EGA_RES_HEIGHT - 14 + MIN(13, MAX(0, frameTimes[i] - 10)), color);
   }
}

DWORD WINAPI _renderThread(LPVOID lpParam) {
   ThreadData *data = lpParam;
   iDeviceContextInitRendering(data->renderer->context);
   egaTextureInitOGL(data->renderer->tex);

   while (data->running) {
      App *app = appGet();
      static Microseconds lastUpdated = 0;
      static int lastFrame = -1;
      
      Microseconds time = appGetTime(app);
      Microseconds deltaTime = time - lastUpdated;
      lastUpdated = time;

      static Rectf vp;
      Scene *scene = data->front;
      if (scene) {;
         _renderFrameTime(scene);

         if (lastFrame == data->frame) {
            int tries = 10;
            while (tries-- && lastFrame == data->frame) {
               Sleep(0);
            }
         }

         if (WaitForSingleObject(data->mutex, INFINITE) == WAIT_OBJECT_0) {
            if (lastFrame == data->frame) {
               _pushFrameResult(scene, deltaTime, DUPE);
               Sleep(8);
            }
            else if (data->frame - lastFrame > 1) {
               _pushFrameResult(scene, deltaTime, DROP);
            }
            else{
               _pushFrameResult(scene, deltaTime, NONE);
            }

            lastFrame = data->frame;            

            egaTextureUpdateTexture(data->renderer->tex, &scene->frame, scene->palette.colors);
            memcpy(&vp, &scene->vp, sizeof(Rectf));
            ReleaseMutex(data->mutex);
         }

         _performRender(data->renderer, &vp);
         iDeviceContextCommitRender(data->renderer->context);

      }
      else {
         Sleep(0);
      }
   }

   egaTextureDestructOGL(data->renderer->tex);
   return 0;
}

void _Init(OGLRenderer *self) {
   //init the thread
   self->tex = egaTextureCreate();
   _startThread(self);
}
void _RenderFrame(OGLRenderer *self, Frame *frame, byte *palette, Rectf *vp) {
	//swap frames and palettes
   Scene *scene = self->thread->back;
   memcpy(&scene->frame, frame, sizeof(Frame));
   memcpy(scene->palette.colors, palette, sizeof(Palette));
   memcpy(&scene->vp, vp, sizeof(Rectf));

   if (WaitForSingleObject(self->thread->mutex, INFINITE) == WAIT_OBJECT_0) {
      Scene *temp = self->thread->back;
      self->thread->back = self->thread->front;
      self->thread->front = temp;

      _InterlockedExchange(&self->thread->frame, self->thread->frame+1);

      ReleaseMutex(self->thread->mutex);
   }
}
void _Destroy(OGLRenderer *self){
   //close the thread and join
   _InterlockedExchange(&self->thread->running, false);
   WaitForSingleObject(self->thread->handle, INFINITE);
   CloseHandle(self->thread->handle);
   CloseHandle(self->thread->mutex);

   checkedFree(self->thread);
   egaTextureDestroy(self->tex);
   checkedFree(self);
}