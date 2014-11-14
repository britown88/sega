#pragma once

#include "segalib\EGA.h"
#include "segautils\Rect.h"

typedef struct IRenderer_t IRenderer;

typedef struct {
   void(*init)(IRenderer*);
   void(*renderFrame)(IRenderer*, Frame *, byte *, Rectf *);
   void(*destroy)(IRenderer*);
} IRendererVTable;

struct IRenderer_t{
   IRendererVTable *vTable;
};

void iRendererInit(IRenderer*self);
void iRendererRenderFrame(IRenderer*self, Frame *frame, byte *palette, Rectf *viewport);
void iRendererDestroy(IRenderer*self);

