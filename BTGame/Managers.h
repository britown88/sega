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
typedef struct CombatManager_t CombatManager;
typedef struct Frame_t Frame;
typedef struct WorldView_t WorldView;
typedef struct DestructionManager_t DestructionManager;
typedef struct AIManager_t AIManager;

typedef struct BTManagers_t {
   RenderManager *renderManager;
   CursorManager *cursorManager;
   GridManager *gridManager;
   CommandManager *commandManager;
   InterpolationManager *interpolationManager;
   DiceManager *diceManager;
   SelectionManager *selectionManager;
   LogManager *logManager;
   CombatManager *combatManager;
   DestructionManager *destructionManager;
   AIManager *AIManager;
}BTManagers;

RenderManager *createRenderManager(WorldView *view, double *fps);
void renderManagerRender(RenderManager *self, Frame *frame);

CommandManager *createCommandManager(WorldView *view);
void commandManagerUpdate(CommandManager *self);
Action *commandManagerCreateAction(CommandManager *self);
float commandManagerGetRoutineRange(CommandManager *self, StringView cmdID);
float commandManagerGetSlotRange(CommandManager *self, Entity *e, size_t slot);
bool entityCommandQueueEmpty(Entity *e);
bool entityShouldAutoAttack(Entity *e);
void entityPauseCommand(Entity *e, Action *cmd);
void entityPushCommand(Entity *e, Action *cmd);
void entityPushFrontCommand(Entity *e, Action *cmd);
void entityCancelFirstCommand(Entity *e);
void entityForceCancelFirstCommand(Entity *e);
void entityCancelAllCommands(Entity *e);
void entityForceCancelAllCommands(Entity *e);
void entityClearCommands(Entity *e);

CursorManager *createCursorManager(WorldView *view);
void cursorManagerCreateCursor(CursorManager *self);
void cursorManagerStartDrag(CursorManager *self, int x, int y);
Recti cursorManagerEndDrag(CursorManager *self, int x, int y);
void cursorManagerUpdate(CursorManager *self, int x, int y);

InterpolationManager *createInterpolationManager(WorldView *view);
void interpolationManagerUpdate(InterpolationManager *self);

DiceManager *createDiceManager(WorldView *view);
void diceManagerUpdate(DiceManager *self);

DestructionManager *createDestructionManager(WorldView *view);
void destructionManagerUpdate(DestructionManager *self);

AIManager *createAIManager(WorldView *view);
void AIManagerUpdate(AIManager *self);




