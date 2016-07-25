#pragma once

#include "segautils\Preprocessor.h"
#include "segautils/Defs.h"
#include "segautils/Rect.h"
#include "segashared/Strings.h"
#include "Verbs.h"
#include "RenderLayers.h"


typedef struct RenderManager_t RenderManager;
typedef struct CursorManager_t CursorManager;
typedef struct GridManager_t GridManager;
typedef struct InterpolationManager_t InterpolationManager;
typedef struct WaitManager_t WaitManager;
typedef struct GridMovementManager_t GridMovementManager;
typedef struct PCManager_t PCManager;
typedef struct TextBoxManager_t TextBoxManager;
typedef struct VerbManager_t VerbManager;
typedef struct ActorManager_t ActorManager;
typedef struct ClockManager_t ClockManager;

typedef struct Frame_t Frame;
typedef struct WorldView_t WorldView;
typedef struct Entity_t Entity;

typedef struct BTManagers_t {
   RenderManager *renderManager;
   CursorManager *cursorManager;
   GridManager *gridManager;
   InterpolationManager *interpolationManager;
   WaitManager *waitManager;
   GridMovementManager *gridMovementManager;   
   PCManager *pcManager;
   TextBoxManager *textBoxManager;
   VerbManager *verbManager;
   ActorManager *actorManager;
   ClockManager *clockManager;
}BTManagers;

#define ClosureTPart \
    CLOSURE_RET(void) /*return edge*/\
    CLOSURE_NAME(LayerRenderer) \
    CLOSURE_ARGS(Frame *)
#include "segautils\Closure_Decl.h"

RenderManager *createRenderManager(WorldView *view, double *fps);
void renderManagerInitialize(RenderManager *self);
void renderManagerRender(RenderManager *self, Frame *frame);
void renderManagerToggleFPS(RenderManager *self);
void renderManagerAddLayerRenderer(RenderManager *self, Layer l, LayerRenderer renderer);
void renderManagerRemoveLayerRenderer(RenderManager *self, Layer l);


CursorManager *createCursorManager(WorldView *view);
void cursorManagerCreateCursor(CursorManager *self);
void cursorManagerUpdate(CursorManager *self, int x, int y);
void cursorManagerSetVerb(CursorManager *self, Verbs v);
void cursorManagerClearVerb(CursorManager *self);

ActorManager *createActorManager(WorldView *view);
void actorManagerUpdate(ActorManager *self);
void actorManagerClearErrorFlag(ActorManager *self);//resumes execution of script steps if it was halted by an error

ClockManager *createClockManager(WorldView *view);
void clockManagerUpdate(ClockManager *self);

InterpolationManager *createInterpolationManager(WorldView *view);
void interpolationManagerUpdate(InterpolationManager *self);

WaitManager *createWaitManager(WorldView *view);
void waitManagerUpdate(WaitManager *self);

GridMovementManager *createGridMovementManager(WorldView *view);
void gridMovementManagerUpdate(GridMovementManager *self);
void gridMovementManagerStopEntity(GridMovementManager *self, Entity *e);
void gridMovementManagerMoveEntity(GridMovementManager *self, Entity *e, short x, short y);
void gridMovementManagerMoveEntityRelative(GridMovementManager *self, Entity *e, short x, short y);
bool gridMovementManagerEntityIsMoving(GridMovementManager *self, Entity *e);
int gridDistance(int x0, int y0, int x1, int y1);

//if an entity is moving, this returns the cell ID they were in befofre their last movement
//returns INF if not moving, useful for drawing
size_t gridMovementManagerGetEntityLastPosition(GridMovementManager *self, Entity *e);

PCManager *createPCManager(WorldView *view);
void pcManagerUpdate(PCManager *self);
void pcManagerCreatePC(PCManager *self);
void pcManagerStop(PCManager *self);
void pcManagerMove(PCManager *self, short x, short y);
void pcManagerMoveRelative(PCManager *self, short x, short y);
void pcManagerToggleTorch(PCManager *self);
void pcManagerSetTorch(PCManager *self, bool torchOn);
void pcManagerSetSneak(PCManager *self, bool sneaking);

TextBoxManager *createTextBoxManager(WorldView *view);
void textBoxManagerCreateTextBox(TextBoxManager *self, StringView name, Recti area);
int textBoxManagerPushText(TextBoxManager *self, StringView name, const char *msg);
void textBoxManagerUpdate(TextBoxManager *self);
int textBoxManagerSetTextAreaVisibility(TextBoxManager *self, StringView name, bool visible);
int textBoxManagerHideTextArea(TextBoxManager *self, StringView name);
int textBoxManagerShowTextArea(TextBoxManager *self, StringView name);



