#include "BT.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"
#include "segashared\Strings.h"

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
#include "DB.h"
#include "assets.h"
#include "Weather.h"
#include "TextArea.h"
#include "Actors.h"
#include "RenderHelpers.h"
#include "Sprites.h"

typedef struct {
   VirtualApp vApp;
   AppData data;

   RenderManager *renderManager;
   CursorManager *cursorManager;
   GridManager *gridManager;
   PCManager *pcManager;
   VerbManager *verbManager;
   ActorManager *actorManager;
   TextAreaManager *textAreaManager;

   FramerateViewer *framerateViewer;
   FontFactory *fontFactory;
   ImageLibrary *imageLibrary;
   SpriteManager *spriteManager;
   FSM *gameState;
   Viewport viewport;
   GridSolver *gridSolver;
   WorldView view;
   Console *console;
   ChoicePrompt *choicePrompt;
   DB_assets *db;
   Weather *weather;

   lua_State *L;

   vec(ActorPtr) *testActors;

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

#define CREATE_AND_VIEW(NAME, CALL) r->NAME = CALL; \
   r->view.NAME = r->NAME;

VirtualApp *btCreate() {
   BTGame *r = checkedCalloc(1, sizeof(BTGame));
   r->vApp.vTable = getVtable();
   r->data = createData();
   
   r->viewport = (Viewport){   
      .region = { GRID_POS_X, GRID_POS_Y, GRID_PX_WIDTH, GRID_PX_HEIGHT },
      .worldPos = { 0, 0 } 
   };
   r->view.viewport = &r->viewport;

   r->view.gameClock = gameClockGet();

   CREATE_AND_VIEW(imageLibrary, imageLibraryCreate(&r->view));
   CREATE_AND_VIEW(spriteManager, spriteManagerCreate(&r->view));
   CREATE_AND_VIEW(gameState, fsmCreate());

   CREATE_AND_VIEW(cursorManager, cursorManagerCreate(&r->view));
   CREATE_AND_VIEW(gridManager, gridManagerCreate(&r->view));
   CREATE_AND_VIEW(pcManager, pcManagerCreate(&r->view));
   CREATE_AND_VIEW(verbManager, verbManagerCreate(&r->view));
   CREATE_AND_VIEW(actorManager, actorManagerCreate(&r->view));
   CREATE_AND_VIEW(textAreaManager, textAreaManagerCreate(&r->view));

   CREATE_AND_VIEW(framerateViewer, framerateViewerCreate(&r->view, &r->data.fps));
   CREATE_AND_VIEW(gridSolver, gridSolverCreate(&r->view));
   CREATE_AND_VIEW(console, consoleCreate(&r->view));
   CREATE_AND_VIEW(L, luaCreate());
   CREATE_AND_VIEW(db, db_assetsCreate());
   CREATE_AND_VIEW(weather, createWeather(&r->view));
   CREATE_AND_VIEW(choicePrompt, createChoicePrompt(&r->view));

   r->testActors = vecCreate(ActorPtr)(&actorPtrDestroy);

   return (VirtualApp*)r;
}

#undef CREATE_AND_VIEW

void _destroy(BTGame *self){
   fsmDestroy(self->gameState);

   consoleDestroy(self->console);
   self->console = self->view.console = NULL;  

   vecDestroy(ActorPtr)(self->testActors);

   verbManagerDestroy(self->verbManager);
   cursorManagerDestroy(self->cursorManager);
   
   
   pcManagerDestroy(self->pcManager);
   gridManagerDestroy(self->gridManager);
   actorManagerDestroy(self->actorManager);

   fontFactoryDestroy(self->fontFactory);
   framerateViewerDestroy(self->framerateViewer);
   spriteManagerDestroy(self->spriteManager);
   imageLibraryDestroy(self->imageLibrary);
   gridSolverDestroy(self->gridSolver);
   choicePromptDestroy(self->choicePrompt);
   db_assetsDestroy(self->db);
   weatherDestroy(self->weather);
   textAreaManagerDestroy(self->textAreaManager);
   
   luaDestroy(self->L);
   checkedFree(self);
}

static void _addActor(BTGame *app, int x, int y, int imgX, int imgY) {

   Actor *a =  actorManagerCreateActor(app->actorManager);

   actorSetImage(a, stringIntern(IMG_TILE_ATLAS));   
   actorSetImagePos(a, (Int2) { imgX, imgY });

   actorSetGridPosition(a, (Int2) { x, y });
   actorSnap(a);

   vecPushBack(ActorPtr)(app->testActors, &a);
}

static void _addTestEntities(BTGame *app) {
   int i;

   {
      StringView boxName = stringIntern("smallbox");
      TextArea *area = textAreaCreate(15, 22, 23, 2);
      textAreaPushText(area, "You are likely to be eaten by a [c=0,13]grue[/c].");
      textAreaSetSpeed(area, 50);
      textAreaManagerRegister(app->view.textAreaManager, boxName, area);

   }

   for (i = 0; i < 10; ++i) {
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

   luaLoadAllLibraries(self->L, &self->view);
   luaLoadAssets(self->L);

   //gonna do our initial db connection here
   if (dbConnect((DBBase*)self->db, "chronicles.db", false) != DB_SUCCESS){
#ifdef _DEBUG
      if (dbConnect((DBBase*)self->db, "chronicles.db", true) != DB_SUCCESS) {
         SEGASSERT(true);
      }

      db_assetsCreateTables(self->db);
      luaBuildDB(self->L);
#else
      SEGASSERT(true);
#endif
   }

   //sesneless default palette woohoo
   {
      Palette defPal = { {0, 1, 2, 3, 4, 58, 20, 7, 56, 57, 60, 62, 63, 60, 62, 63} };
      appSetPalette(appGet(), &defPal);
   }

   //load our font factory fopr the font rendering
   self->fontFactory = initFontFactory(&self->view);
   self->view.fontFactory = self->fontFactory;

   cursorManagerCreateCursor(self->cursorManager);
   pcManagerCreatePC(self->pcManager);

   gridManagerSetAmbientLight(self->gridManager, MAX_BRIGHTNESS);

   _addTestEntities(self);

   //push the opening state
   fsmPush(self->gameState, gameStateCreateSplash(&self->view));
   
}


void _onStep(BTGame *self){
   fsmSend(self->gameState, GameStateHandleInput);
   fsmSend(self->gameState, GameStateUpdate);
   fsmSendData(self->gameState, GameStateRender, self->vApp.currentFrame);
}



