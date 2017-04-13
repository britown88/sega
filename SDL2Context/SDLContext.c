#include "SDLContext.h"

#include "SEGA\IDeviceContext.h"
#include "SEGA\App.h"

#include "segautils/IncludeWindows.h"
#include "segashared/CheckedMemory.h"
#include "SEGA\Input.h"

#include <stdio.h>

#define ClosureTPart CLOSURE_NAME(MousePos)
#include "segautils\Closure_Impl.h"

#include "GL/glew.h"
#include "SDL2/SDL.h"


typedef struct {
   IDeviceContext context;
   SDL_Window *window;
   SDL_GLContext sdlContext;

   LARGE_INTEGER clock_freq, clock_start;
   Int2 size;
   bool shouldClose;
}SDLContext;

static int _init(SDLContext *self, int width, int height, StringView winTitle, int flags);
static void _initRendering(SDLContext *self);
static void _commitRender(SDLContext *self);
static void _preRender(SDLContext *self);
static void _postRender(SDLContext *self);
static int _shouldClose(SDLContext *self);
static Int2 _windowSize(SDLContext *self);
static Microseconds _time(SDLContext *self);
static void _destroy(SDLContext *self);
static Keyboard* _keyboard(SDLContext *self);
static Mouse* _mouse(SDLContext *self);

static IDeviceContextVTable *_getTable() {
   static IDeviceContextVTable *out = NULL;
   if (!out) {
      out = calloc(1, sizeof(IDeviceContextVTable));
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

IDeviceContext *createSDLContext() {
   SDLContext *out = checkedCalloc(1, sizeof(SDLContext));
   out->context.vTable = _getTable();
   return (IDeviceContext *)out;
}

int _init(SDLContext *self, int width, int height, StringView winTitle, int flags) {
   self->size = (Int2){ width, height };

   SDL_Init(SDL_INIT_VIDEO);
   self->window = SDL_CreateWindow("CRN4.EXE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height, SDL_WINDOW_OPENGL);

   self->sdlContext = SDL_GL_CreateContext(self->window);

   // Set our OpenGL version.
   // SDL_GL_CONTEXT_CORE gives us only the newer version, deprecated functions are disabled
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

   // 3.2 is part of the modern versions of OpenGL, but most video cards whould be able to run it
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

   // Turn on double buffering with a 24bit Z buffer.
   // You may need to change this to 16 or 32 for your system
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

   SDL_GL_SetSwapInterval(0);

   timeBeginPeriod(1);
   QueryPerformanceFrequency(&self->clock_freq);
   QueryPerformanceCounter(&self->clock_start);
   
   return 0;
}

void _initRendering(SDLContext *self) {
   //glWindowBeginRendering(self->window);   
}
void _commitRender(SDLContext *self) {
   //glWindowSwapBuffers(self->window);
   SDL_GL_SwapWindow(self->window);
}
void _preRender(SDLContext *self) {

}
void _postRender(SDLContext *self) {
   //glWindowPollEvents(self->window);

   SDL_Event event;
   while (SDL_PollEvent(&event))
   {
      if (event.type == SDL_QUIT) {
         self->shouldClose = true;
      }

   }
}
int _shouldClose(SDLContext *self) {
   return self->shouldClose;
}
Int2 _windowSize(SDLContext *self) {
   return self->size;
}

Microseconds _time(SDLContext *self) {
   LARGE_INTEGER end, elapsed;
   QueryPerformanceCounter(&end);

   elapsed.QuadPart = end.QuadPart - self->clock_start.QuadPart;
   elapsed.QuadPart *= 1000000;
   elapsed.QuadPart /= self->clock_freq.QuadPart;

   return elapsed.QuadPart;
   //Uint32 ticks = SDL_GetTicks();
   //uint64_t ticks64 = (uint64_t)ticks;
   //ticks64 *= 1000;
   //return  ticks64;
}
void _destroy(SDLContext *self) {
   // Delete our OpengL context
   SDL_GL_DeleteContext(self->sdlContext);

   // Destroy our window
   SDL_DestroyWindow(self->window);

   // Shutdown SDL 2
   SDL_Quit();
   checkedFree(self);
}
Keyboard *_keyboard(SDLContext *self) {
   static Keyboard *k = 0;
   if (!k) { k = keyboardCreate(); }
   return k;
}
static Int2 _getMousePos(SDLContext *win) {
   return (Int2) { 0, 0 };
}

Mouse* _mouse(SDLContext *self) {
   static Mouse *m = 0;
   if (!m) { 
      MousePos getPos;
      closureInit(MousePos)(&getPos, self->window, (MousePosFunc)&_getMousePos, NULL);
      m = mouseCreate(getPos);
   }
   return m;
}

