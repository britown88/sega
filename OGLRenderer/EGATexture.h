#pragma once

#include "segalib\EGA.h"

typedef struct EGATexture_t EGATexture;

EGATexture *egaTextureCreate();
void egaTextureDestroy(EGATexture *self);

void egaTextureRenderFrame(EGATexture *self, Frame *frame, byte *palette);


