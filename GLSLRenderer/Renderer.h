#pragma once

#include "segautils\Rect.h"
#include "segautils\Matrix.h"

typedef struct Renderer_t Renderer;

Renderer *rendererCreate();
void rendererDestroy(Renderer *self);

void rendererPushViewport(Renderer *self, Rectf bounds);
void rendererPopViewport(Renderer *self);

void rendererPushCamera(Renderer *self, Rectf bounds);
void rendererPopCamera(Renderer *self);

Matrix *rendererGetViewMatrix(Renderer *self);
Rectf *rendererGetViewport(Renderer *self);

