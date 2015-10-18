#pragma once

#include "segalib\EGA.h"

typedef struct EGATexture_t EGATexture;

EGATexture *egaTextureCreate();
void egaTextureInitOGL(EGATexture *self);
void egaTextureDestructOGL(EGATexture *self);
void egaTextureDestroy(EGATexture *self);

void egaTextureRenderFrame(EGATexture *self);
void egaTextureUpdateTexture(EGATexture *self, Frame *frame, byte *palette);


