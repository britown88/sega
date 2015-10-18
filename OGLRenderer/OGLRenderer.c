#include "defined_gl.h"
#include "OGLRenderer.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "SEGA/App.h"

#include "EGATexture.h"

#include <windows.h>
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

DWORD WINAPI _renderThread(LPVOID lpParam) {
   ThreadData *data = lpParam;

   iDeviceContextInitRendering(data->renderer->context);
   egaTextureInitOGL(data->renderer->tex);

   while (data->running) {
      static Rectf vp;
      Scene *scene = data->front;
      if (scene) {
         if (WaitForSingleObject(data->mutex, INFINITE) == WAIT_OBJECT_0) {
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