#pragma once

#include "segautils/Defs.h"

typedef struct WorldView_t WorldView;
typedef struct FontFactory_t FontFactory;
typedef struct Frame_t Frame;
typedef struct Span_t Span;

FontFactory *initFontFactory(WorldView *view);
void frameRenderSpan(WorldView *view, Frame *frame, byte *x, byte *y, Span *span);
