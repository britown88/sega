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

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
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

   RegisterManager(self->managers.renderManager, createRenderManager(self->entitySystem, self->imageLibrary, &self->data.fps));
   RegisterManager(self->managers.cursorManager, createCursorManager(self->entitySystem));
   RegisterManager(self->managers.gridManager, createGridManager(self->entitySystem));
   RegisterManager(self->managers.commandManager, createCommandManager(self->entitySystem));
   RegisterManager(self->managers.interpolationManager, createInterpolationManager(self->entitySystem));
   RegisterManager(self->managers.diceManager, createDiceManager(self->entitySystem));
   RegisterManager(self->managers.selectionManager, createSelectionManager(self->entitySystem));
}

void _destroyEntitySystem(BTGame *self){
   entitySystemDestroy(self->entitySystem);

   managerDestroy((Manager*)self->managers.renderManager);
   managerDestroy((Manager*)self->managers.cursorManager);
   managerDestroy((Manager*)self->managers.gridManager);
   managerDestroy((Manager*)self->managers.commandManager);
   managerDestroy((Manager*)self->managers.interpolationManager);
   managerDestroy((Manager*)self->managers.diceManager);
   managerDestroy((Manager*)self->managers.selectionManager);
}

VirtualApp *btCreate() {
   BTGame *r = checkedCalloc(1, sizeof(BTGame));
   r->vApp.vTable = getVtable();
   r->data = createData(); 

   //Other constructor shit goes here   
   r->imageLibrary = imageLibraryCreate();
   r->gameState = fsmCreate();
   _initEntitySystem(r);

   //build the public view
   r->view = (WorldView){
      .managers = &r->managers,
      .entitySystem = r->entitySystem,
      .imageLibrary = r->imageLibrary,
      .gameState = r->gameState
   };

   return (VirtualApp*)r;
}

void _destroy(BTGame *self){
   fsmDestroy(self->gameState);
   _destroyEntitySystem(self);

   imageLibraryDestroy(self->imageLibrary);
   checkedFree(self);
}

void _onStart(BTGame *self){
   int i; 
   {
      Entity *e = entityCreate(self->entitySystem);
      
      COMPONENT_ADD(e, PositionComponent, 0, 0);
      COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/boardui.ega"));
      entityUpdate(e);
   }   

   for (i = 0; i < 24/*CELL_COUNT - 12*/; ++i){

      Entity *e = entityCreate(self->entitySystem);

      COMPONENT_ADD(e, PositionComponent, 0, 0);
      COMPONENT_ADD(e, ImageComponent, stringIntern(i % 2 ? "assets/img/actor.ega" : "assets/img/badguy.ega"));

      COMPONENT_ADD(e, LayerComponent, LayerTokens);;
      COMPONENT_ADD(e, GridComponent, i % TABLE_WIDTH, i/TABLE_WIDTH);
      COMPONENT_ADD(e, SizeComponent, 32, 32);
      COMPONENT_ADD(e, TeamComponent, i%2);
      //COMPONENT_ADD(e, WanderComponent, 1);

      entityUpdate(e);
   }

   appLoadPalette(appGet(), "assets/img/boardui.pal");
   cursorManagerCreateCursor(self->managers.cursorManager);

   //push the opening state
   fsmPush(self->gameState, gameStateCreateBoard(&self->view));

}


void _onStep(BTGame *self){
   fsmSend(self->gameState, GameStateUpdate);
   fsmSend(self->gameState, GameStateHandleInput);
   fsmSendData(self->gameState, GameStateRender, self->vApp.currentFrame);
         
}



