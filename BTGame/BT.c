#include "BT.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"
#include "segashared\Strings.h"
#include "CoreComponents.h"
#include "Managers.h"
#include "ImageLibrary.h"
#include "GridManager.h"
#include "SelectionManager.h"
#include "WorldView.h"
#include "GameState.h"
#include "LogManager.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 720
#define FULLSCREEN 0
#define FRAME_RATE 60.0

typedef struct {
   VirtualApp vApp;
   AppData data;
   BTManagers managers;
   EntitySystem *entitySystem;
   ImageLibrary *imageLibrary;
   FSM *gameState;

   WorldView view;

} BTGame;

#pragma region App_Things

static AppData *_getData(BTGame *);
static void _destroy(BTGame *);
static void _onStart(BTGame *);
static void _onStep(BTGame *);

static VirtualAppVTable *getVtable()
{
   static VirtualAppVTable *vtable;
   if(!vtable) {
      vtable = malloc(sizeof(VirtualAppVTable));
      vtable->getData = (AppData *(*)(VirtualApp *))&_getData;
      vtable->destroy = (void (*)(VirtualApp *))&_destroy;
      vtable->onStart = (void (*)(VirtualApp *))&_onStart;
      vtable->onStep = (void (*)(VirtualApp *))&_onStep;
   }

   return vtable;
}
AppData createData() {
   AppData data = { 0 };

   data.defaultWindowSize = (Int2){ WINDOW_WIDTH, WINDOW_HEIGHT };
   data.frameRate = FRAME_RATE;

   if (FULLSCREEN){
      data.dcFlags |= DC_FLAG_FULLSCREEN;
   }

   data.windowTitle = stringIntern("sEGA: An elegant weapon for a more civilized age.");

   return data;
}
AppData *_getData(BTGame *self) {
   return &self->data;
}

#pragma endregion

#define RegisterManager(member, funcCall) \
   member = funcCall; \
   entitySystemRegisterManager(self->entitySystem, (Manager*)member);

void _initEntitySystem(BTGame *self){
   self->entitySystem = entitySystemCreate();
   self->view.entitySystem = self->entitySystem;

   RegisterManager(self->managers.renderManager, createRenderManager(&self->view, &self->data.fps));
   RegisterManager(self->managers.cursorManager, createCursorManager(&self->view));
   RegisterManager(self->managers.gridManager, createGridManager(&self->view));
   RegisterManager(self->managers.commandManager, createCommandManager(&self->view));
   RegisterManager(self->managers.interpolationManager, createInterpolationManager(&self->view));
   RegisterManager(self->managers.diceManager, createDiceManager(&self->view));
   RegisterManager(self->managers.selectionManager, createSelectionManager(&self->view));
   RegisterManager(self->managers.logManager, createLogManager(&self->view));
}

void _destroyEntitySystem(BTGame *self){
   entitySystemDestroy(self->entitySystem);
}

VirtualApp *btCreate() {
   BTGame *r = checkedCalloc(1, sizeof(BTGame));
   r->vApp.vTable = getVtable();
   r->data = createData();

   //Other constructor shit goes here   
   r->imageLibrary = imageLibraryCreate();
   r->gameState = fsmCreate();  

   r->view.imageLibrary = r->imageLibrary;
   r->view.gameState = r->gameState;
   r->view.managers = &r->managers;

   _initEntitySystem(r);
   

   return (VirtualApp*)r;
}

void _destroy(BTGame *self){
   fsmDestroy(self->gameState);
   _destroyEntitySystem(self);

   imageLibraryDestroy(self->imageLibrary);
   checkedFree(self);
}

void _onStart(BTGame *self){

   Entity *e = entityCreate(self->entitySystem);      
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/boardui.ega"));
   entityUpdate(e);

   appLoadPalette(appGet(), "assets/img/default.pal");
   cursorManagerCreateCursor(self->managers.cursorManager);

   //push the opening state
   fsmPush(self->gameState, gameStateCreateBoard(&self->view));

}


void _onStep(BTGame *self){
   fsmSend(self->gameState, GameStateHandleInput);
   fsmSend(self->gameState, GameStateUpdate);   
   fsmSendData(self->gameState, GameStateRender, self->vApp.currentFrame);         
}



