#pragma once

#include "segautils\Preprocessor.h"
#include "segautils\Rect.h"
#include "segautils\Coroutine.h"
#include "Entities\Entities.h"
#include "Actions.h"

typedef struct RenderManager_t RenderManager;
typedef struct CursorManager_t CursorManager;
typedef struct EntitySystem_t EntitySystem;
typedef struct ImageLibrary_t ImageLibrary;
typedef struct GridManager_t GridManager;
typedef struct CommandManager_t CommandManager;
typedef struct InterpolationManager_t InterpolationManager;
typedef struct DiceManager_t DiceManager;
typedef struct SelectionManager_t SelectionManager;
typedef struct LogManager_t LogManager;
typedef struct Frame_t Frame;

typedef struct BTManagers_t {
   RenderManager *renderManager;
   CursorManager *cursorManager;
   GridManager *gridManager;
   CommandManager *commandManager;
   InterpolationManager *interpolationManager;
   DiceManager *diceManager;
   SelectionManager *selectionManager;
   LogManager *logManager;
}BTManagers;

RenderManager *createRenderManager(EntitySystem *system, ImageLibrary *imageManager, double *fps);
void renderManagerRender(RenderManager *self, Frame *frame);

CommandManager *createCommandManager(EntitySystem *system, GridManager *gridManager);
void commandManagerUpdate(CommandManager *self);
Action *commandManagerCreateAction(CommandManager *self);
void entityPushCommand(Entity *e, Action *cmd);
void entityCancelCommands(Entity *e);
void entityClearCommands(Entity *e);

CursorManager *createCursorManager(EntitySystem *system);
void cursorManagerCreateCursor(CursorManager *self);
void cursorManagerStartDrag(CursorManager *self, int x, int y);
Recti cursorManagerEndDrag(CursorManager *self, int x, int y);
void cursorManagerUpdate(CursorManager *self, int x, int y);

InterpolationManager *createInterpolationManager(EntitySystem *system);
void interpolationManagerUpdate(InterpolationManager *self);
void interpolationManagerPause(InterpolationManager *self);
void interpolationManagerResume(InterpolationManager *self);

DiceManager *createDiceManager(EntitySystem *system);
void diceManagerUpdate(DiceManager *self);




