#pragma once

//cant believe im making this but here goes

#include "segautils/Defs.h"
#include "segautils/Rect.h"

typedef struct LightDebugger_t LightDebugger;
typedef struct WorldView_t WorldView;
typedef struct Texture_t Texture;

LightDebugger *lightDebuggerCreate(WorldView *view);
void lightDebuggerDestroy(LightDebugger *self);

void lightDebuggerStartNewSet(LightDebugger *self, Recti source, Recti target);
void lightDebuggerAddRay(LightDebugger *self, Int2 p1, Int2 p2, bool blocked);
void lightDebuggerClear(LightDebugger *self);

void lightDebuggerRender(LightDebugger *self, Texture *tex);