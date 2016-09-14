#include "pch.h"
#include "Renderer.h"

#include "SEGA\IDeviceContext.h"
#include "SEGA\IRenderer.h"
#include "SEGA\App.h"

#include "segashared/CheckedMemory.h"



typedef struct {
   IRenderer ir;
   IDeviceContext *context;
   UWP::UWPMain *main;
} UWPRenderer;

static void _Init(UWPRenderer*);
static void _RenderFrame(UWPRenderer*, Frame *, byte *, Rectf *);
static void _Destroy(UWPRenderer*);

static IRendererVTable *_getTable() {
   static IRendererVTable *r = 0;
   if (!r) {
      r = (IRendererVTable*)calloc(1, sizeof(IRendererVTable));
      r->init = (void(*)(IRenderer*))&_Init;
      r->renderFrame = (void(*)(IRenderer*, Frame*, byte*, Rectf *))&_RenderFrame;
      r->destroy = (void(*)(IRenderer*))&_Destroy;
   }

   return r;
}

IRenderer *createUWPRenderer(IDeviceContext *dc, UWP::UWPMain *main) {
   UWPRenderer *r = (UWPRenderer*)checkedCalloc(1, sizeof(UWPRenderer));
   r->ir.vTable = _getTable();
   r->context = dc;
   r->main = main;

   return (IRenderer *)r;
}

void _Init(UWPRenderer *self) {
   //init the thread
}
void _RenderFrame(UWPRenderer *self, Frame *frame, byte *palette, Rectf *vp) {
   //swap frames and palettes

   self->main->RenderEGA();
}
void _Destroy(UWPRenderer *self) {
   checkedFree(self);
}