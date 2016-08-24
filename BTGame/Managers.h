#pragma once

#include "segautils\Preprocessor.h"
#include "segautils/Defs.h"
#include "segautils/Rect.h"
#include "segashared/Strings.h"
#include "Verbs.h"
#include "RenderLayers.h"
#include "RichText.h"


typedef struct RenderManager_t RenderManager;
typedef struct CursorManager_t CursorManager;
typedef struct GridManager_t GridManager;
typedef struct PCManager_t PCManager;
typedef struct VerbManager_t VerbManager;
typedef struct ActorManager_t ActorManager;
typedef struct ClockManager_t ClockManager;
typedef struct FramerateViewer_t FramerateViewer;

typedef struct Frame_t Frame;
typedef struct WorldView_t WorldView;
typedef struct Actor_t Actor;

FramerateViewer *framerateViewerCreate(WorldView *view, double *fps);
void framerateViewerDestroy(FramerateViewer *self);
void framerateViewerToggle(FramerateViewer *self);
void framerateViewerRender(FramerateViewer *self, Frame *frame);

CursorManager *cursorManagerCreate(WorldView *view);
void cursorManagerDestroy(CursorManager *self);
void cursorManagerCreateCursor(CursorManager *self);
void cursorManagerUpdate(CursorManager *self, int x, int y);
void cursorManagerSetVerb(CursorManager *self, Verbs v);
void cursorManagerClearVerb(CursorManager *self);
void cursorManagerRender(CursorManager *self, Frame *frame);

PCManager *pcManagerCreate(WorldView *view);
void pcManagerDestroy(PCManager *self);

void pcManagerUpdate(PCManager *self);
void pcManagerCreatePC(PCManager *self);
void pcManagerStop(PCManager *self);
void pcManagerMove(PCManager *self, short x, short y);
void pcManagerMoveRelative(PCManager *self, short x, short y);
void pcManagerToggleTorch(PCManager *self);
void pcManagerSetTorch(PCManager *self, bool torchOn);
void pcManagerSetSneak(PCManager *self, bool sneaking);







