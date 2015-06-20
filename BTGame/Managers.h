#pragma once

#include "segautils\Preprocessor.h"

typedef struct RenderManager_t RenderManager;
typedef struct CursorManager_t CursorManager;
typedef struct Frame_t Frame;
typedef struct WorldView_t WorldView;

typedef struct BTManagers_t {
   RenderManager *renderManager;
   CursorManager *cursorManager;
}BTManagers;

RenderManager *createRenderManager(WorldView *view, double *fps);
void renderManagerRender(RenderManager *self, Frame *frame);

CursorManager *createCursorManager(WorldView *view);
void cursorManagerCreateCursor(CursorManager *self);
void cursorManagerUpdate(CursorManager *self, int x, int y);



