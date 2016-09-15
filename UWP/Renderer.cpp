#include "pch.h"
#include "Renderer.h"

#include "SEGA\IDeviceContext.h"
#include "SEGA\IRenderer.h"
#include "SEGA\App.h"

#include "segashared/CheckedMemory.h"

#include "segautils/BitTwiddling.h"

typedef struct {
   IRenderer ir;
   IDeviceContext *context;
   UWP::UWPMain *main;
   byte *pixels;
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
   r->pixels = (byte*)checkedCalloc(1, EGA_RES_WIDTH*EGA_RES_HEIGHT * 4);

   return (IRenderer *)r;
}

void _Init(UWPRenderer *self) {
   //init the thread
}
void _RenderFrame(UWPRenderer *self, Frame *frame, byte *palette, Rectf *vp) {
   //swap frames and palettes
   int x, y, i;
   byte *cData = self->pixels;

   //NEEDS TO BE BGRA

   for (y = 0; y < EGA_RES_HEIGHT; ++y) {
      for (x = 0; x < EGA_RES_WIDTH; ++x) {

         byte color = 0;
         byte cbytes[4] = { 0 };
         int colori = 0;

         for (i = 0; i < EGA_PLANES; ++i) {
            setBit(&color, i, getBitFromArray(frame->planes[i].lines[y].pixels, x));
         }

         colori = getEGAColor(palette[color]);
         memcpy(cbytes, &colori, 4);

         *cData++ = cbytes[2];
         *cData++ = cbytes[1];
         *cData++ = cbytes[0];
         *cData++ = cbytes[3];
      }
   }

   self->main->RenderEGA(self->pixels);
}
void _Destroy(UWPRenderer *self) {
   checkedFree(self->pixels);
   checkedFree(self);
}