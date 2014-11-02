#pragma once

typedef struct FBO_t FBO;
typedef struct Renderer_t Renderer;

FBO *fboCreate(int width, int height);
void fboDestroy(FBO *self);

void fboRender(FBO *self, Renderer *r);
void fboBind(FBO *self);