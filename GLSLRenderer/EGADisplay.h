#pragma once

#include "segalib\EGA.h"
#include "Renderer.h"
#include "EGAPalette.h"

typedef struct EGADisplay_t EGADisplay;

EGADisplay *egaDisplayCreate();
void egaDisplayDestroy(EGADisplay *self);

EGAPalette *egaDisplayInternPalette(EGADisplay *self, byte *palette);
void egaDisplaySetPalette(EGADisplay *self, EGAPalette *p);
void egaDisplayRender(EGADisplay *self, Renderer *r);

void egaDisplayRenderFrame(EGADisplay *self, Frame *fb);