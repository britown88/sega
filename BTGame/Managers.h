#pragma once

#include "segautils\Preprocessor.h"

typedef struct RenderManager_t RenderManager;
typedef struct CursorManager_t CursorManager;
typedef struct EntitySystem_t EntitySystem;
typedef struct ImageLibrary_t ImageLibrary;
typedef struct GridManager_t GridManager;
typedef struct InterpolationManager_t InterpolationManager;
typedef struct DiceManager_t DiceManager;
typedef struct Frame_t Frame;

typedef struct BTManagers_t {
   RenderManager *renderManager;
   CursorManager *cursorManager;
   GridManager *gridManager;
   InterpolationManager *interpolationManager;
   DiceManager *diceManager;
}BTManagers;

RenderManager *createRenderManager(EntitySystem *system, ImageLibrary *imageManager, double *fps);
void renderManagerRender(RenderManager *self, Frame *frame);

CursorManager *createCursorManager(EntitySystem *system);
void cursorManagerCreateCursor(CursorManager *self);
void cursorManagerUpdate(CursorManager *self, int x, int y);

InterpolationManager *createInterpolationManager(EntitySystem *system);
void interpolationManagerUpdate(InterpolationManager *self);
void interpolationManagerPause(InterpolationManager *self);
void interpolationManagerResume(InterpolationManager *self);

DiceManager *createDiceManager(EntitySystem *system);
void diceManagerUpdate(DiceManager *self);
