#pragma once

#include "segautils/Defs.h"

typedef struct WorldView_t WorldView;
typedef struct FontFactory_t FontFactory;
typedef struct Texture_t Texture;
typedef struct Span_t Span;

FontFactory *initFontFactory(WorldView *view);
void textureRenderSpan(WorldView *view, Texture *tex, byte *x, byte *y, Span *span);
