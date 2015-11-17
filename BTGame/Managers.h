#pragma once

#include "segautils\Preprocessor.h"

typedef struct RenderManager_t RenderManager;
typedef struct CursorManager_t CursorManager;
typedef struct GridManager_t GridManager;
typedef struct InterpolationManager_t InterpolationManager;
typedef struct GridMovementManager_t GridMovementManager;
typedef struct PCManager_t PCManager;

typedef struct Frame_t Frame;
typedef struct WorldView_t WorldView;
typedef struct Entity_t Entity;

typedef struct BTManagers_t {
   RenderManager *renderManager;
   CursorManager *cursorManager;
   GridManager *gridManager;
   InterpolationManager *interpolationManager;
   GridMovementManager *gridMovementManager;
   PCManager *pcManager;
}BTManagers;

RenderManager *createRenderManager(WorldView *view, double *fps);
void renderManagerRender(RenderManager *self, Frame *frame);
void renderManagerToggleFPS(RenderManager *self);

CursorManager *createCursorManager(WorldView *view);
void cursorManagerCreateCursor(CursorManager *self);
void cursorManagerUpdate(CursorManager *self, int x, int y);

InterpolationManager *createInterpolationManager(WorldView *view);
void interpolationManagerUpdate(InterpolationManager *self);

GridMovementManager *createGridMovementManager(WorldView *view);
void gridMovementManagerUpdate(GridMovementManager *self);
void gridMovementManagerStopEntity(GridMovementManager *self, Entity *e);
void gridMovementManagerMoveEntity(GridMovementManager *self, Entity *e, short x, short y);
void gridMovementManagerMoveEntityRelative(GridMovementManager *self, Entity *e, short x, short y);

PCManager *createPCManager(WorldView *view);
void pcManagerUpdate(PCManager *self);
void pcManagerCreatePC(PCManager *self);
void pcManagerStop(PCManager *self);
void pcManagerMove(PCManager *self, short x, short y);
void pcManagerMoveRelative(PCManager *self, short x, short y);



