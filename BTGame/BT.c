#include "BT.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"
#include "segashared\Strings.h"

#include "Entities\Entities.h"
#include "CoreComponents.h"
#include "WorldView.h"
#include "GameClock.h"
#include "ImageLibrary.h"
#include "Managers.h"
#include "GameState.h"
#include "GridManager.h"
#include "GridSolver.h"
#include "Verbs.h"
#include "Console.h"
#include "Lua.h"
#include "LightGrid.h"
#include "MapEditor.h"
#include "ChoicePrompt.h"

typedef struct {
   VirtualApp vApp;
   AppData data;

   BTManagers managers;

   EntitySystem *entitySystem;
   ImageLibrary *imageLibrary;
   FSM *gameState;
   GameClock *gameClock;
   Viewport viewport;
   GridSolver *gridSolver;
   WorldView view;
   Console *console;
   MapEditor *mapEditor;
   ChoicePrompt *choicePrompt;

   lua_State *L;

} BTGame;

#pragma region App_Things

static AppData *_getData(BTGame *);
static void _destroy(BTGame *);
static void _onStart(BTGame *);
static void _onStep(BTGame *);

static VirtualAppVTable *getVtable()
{
   static VirtualAppVTable *vtable;
   if (!vtable) {
      vtable = malloc(sizeof(VirtualAppVTable));
      vtable->getData = (AppData *(*)(VirtualApp *))&_getData;
      vtable->destroy = (void(*)(VirtualApp *))&_destroy;
      vtable->onStart = (void(*)(VirtualApp *))&_onStart;
      vtable->onStep = (void(*)(VirtualApp *))&_onStep;
   }

   return vtable;
}
AppData createData() {
   AppData data = { 0 };

   data.defaultWindowSize = (Int2){ WINDOW_WIDTH, WINDOW_HEIGHT };
   data.desiredFrameRate = FRAME_RATE;

   if (FULLSCREEN){
      data.dcFlags |= DC_FLAG_FULLSCREEN;
   }

   data.windowTitle = stringIntern("Chronicles IV: Ebonheim");

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
   self->view.entitySystem = self->entitySystem;//this is important

   RegisterManager(self->managers.renderManager, createRenderManager(&self->view, &self->data.fps));
   RegisterManager(self->managers.cursorManager, createCursorManager(&self->view));
   RegisterManager(self->managers.gridManager, createGridManager(&self->view));
   RegisterManager(self->managers.interpolationManager, createInterpolationManager(&self->view));
   RegisterManager(self->managers.waitManager, createWaitManager(&self->view));
   RegisterManager(self->managers.gridMovementManager, createGridMovementManager(&self->view));
   RegisterManager(self->managers.pcManager, createPCManager(&self->view));
   RegisterManager(self->managers.textBoxManager, createTextBoxManager(&self->view));
   RegisterManager(self->managers.verbManager, createVerbManager(&self->view));
   RegisterManager(self->managers.actorManager, createActorManager(&self->view));
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
   r->gameClock = gameClockCreate();
   
   r->viewport = (Viewport){   
      .region = { GRID_POS_X, GRID_POS_Y, GRID_PX_WIDTH, GRID_PX_HEIGHT },
      .worldPos = { 0, 0 } 
   };

   //init the view
   r->view.imageLibrary = r->imageLibrary;
   r->view.gameState = r->gameState;
   r->view.managers = &r->managers;
   r->view.gameClock = r->gameClock;
   r->view.viewport = &r->viewport;

   _initEntitySystem(r);

   r->gridSolver = gridSolverCreate(&r->view);
   r->view.gridSolver = r->gridSolver;

   r->console = consoleCreate(&r->view);
   r->view.console = r->console;

   r->mapEditor = mapEditorCreate(&r->view);
   r->view.mapEditor = r->mapEditor;

   r->L = luaCreate();

   r->choicePrompt = createChoicePrompt(&r->view);
   r->view.choicePrompt = r->choicePrompt;

   return (VirtualApp*)r;
}

void _destroy(BTGame *self){
   fsmDestroy(self->gameState);

   //kill the console before the entity system tries throwing errors to it!
   consoleDestroy(self->console);
   self->console = self->view.console = NULL;
   
   _destroyEntitySystem(self);

   imageLibraryDestroy(self->imageLibrary);
   gameClockDestroy(self->gameClock);
   gridSolverDestroy(self->gridSolver);
   mapEditorDestroy(self->mapEditor);
   choicePromptDestroy(self->choicePrompt);
   
   luaDestroy(self->L);
   checkedFree(self);
}

static void _addActor(BTGame *app, int x, int y, int imgX, int imgY) {
   Tile *t = gridManagerTileAtXY(app->managers.gridManager, x, y);
   Entity *e = entityCreate(app->entitySystem);
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, SizeComponent, 14, 14);
   COMPONENT_ADD(e, RectangleComponent, 0);

   COMPONENT_ADD(e, ImageComponent, .imgID = stringIntern(IMG_TILE_ATLAS), .partial = true, .x = imgX, .y = imgY, .width = 14, .height = 14);
   COMPONENT_ADD(e, LayerComponent, LayerGrid);
   COMPONENT_ADD(e, InViewComponent, 0);
   COMPONENT_ADD(e, GridComponent, x, y);
   COMPONENT_ADD(e, LightComponent, .radius = 0, .centerLevel = 0, .fadeWidth = 0);
   COMPONENT_ADD(e, ActorComponent, .moveTime = DEFAULT_MOVE_SPEED, .moveDelay = DEFAULT_MOVE_DELAY);

   if (t) {
      t->schema = 3;
      t->collision = 0;
   }

   entityUpdate(e);
}

static void _addTestEntities(BTGame *app) {
   int i;
   Entity *e = entityCreate(app->entitySystem);
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/bg.ega"));
   COMPONENT_ADD(e, LayerComponent, LayerBackground);
   COMPONENT_ADD(e, RenderedUIComponent, 0);
   entityUpdate(e);

   app->view.backgroundEntity = e;

   {
      StringView boxName = stringIntern("smallbox");
      textBoxManagerCreateTextBox(app->managers.textBoxManager, boxName, (Recti) { 15, 22, 38, 24 });
      textBoxManagerPushText(app->managers.textBoxManager, boxName, "You are likely to be eaten by a [c=0,13]grue[/c].");
      textBoxManagerShowTextArea(app->managers.textBoxManager, boxName);

   }

   for (i = 0; i < 15; ++i) {
      int x = appRand(appGet(), 0, 21);
      int y = appRand(appGet(), 0, 11);
      int sprite = appRand(appGet(), 0, 3);

      _addActor(app, x, y, 70 + (sprite * 14), 28);
   }

}



void _onStart(BTGame *self){
   self->view.L = self->L;

   //we need the console alive to print errors!
   consoleCreateLines(self->console);   
   mapEditorInitialize(self->mapEditor);

   luaLoadAllLibraries(self->L, &self->view);

   luaLoadAssets(self->L);

   appLoadPalette(appGet(), "assets/pal/default.pal");
   renderManagerInitialize(self->managers.renderManager);
   cursorManagerCreateCursor(self->managers.cursorManager);
   pcManagerCreatePC(self->managers.pcManager);
   verbManagerCreateVerbs(self->managers.verbManager);

   gridManagerSetAmbientLight(self->managers.gridManager, MAX_BRIGHTNESS);

   _addTestEntities(self);

   //push the opening state
   fsmPush(self->gameState, gameStateCreateWorld(&self->view));
   
}


void _onStep(BTGame *self){
   fsmSend(self->gameState, GameStateHandleInput);
   fsmSend(self->gameState, GameStateUpdate);
   fsmSendData(self->gameState, GameStateRender, self->vApp.currentFrame);
}



