#include "WorldView.h"
#include "Managers.h"
#include "CoreComponents.h"
#include "LightGrid.h"
#include "Console.h"
#include "GameState.h"
#include "GameHelpers.h"
#include "ImageLibrary.h"
#include "MapEditor.h"
#include "DB.h"

#include "Entities\Entities.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "GameClock.h"

#include "segashared\CheckedMemory.h"

#define VP_SPEED 3
#define VP_FAST_SPEED 8

typedef struct {
   WorldView *view;
   bool pop;
}SplashState;

static void _splashStateCreate(SplashState *state) {}
static void _splashStateDestroy(SplashState *self) {
   checkedFree(self);
}

static void _splashUpdate(SplashState*, GameStateUpdate*);
static void _splashHandleInput(SplashState*, GameStateHandleInput*);
static void _splashRender(SplashState*, GameStateRender*);
static void _splashEnter(SplashState*, StateEnter*);
static void _splashExit(SplashState*, StateExit*);

static void _splash(SplashState *state, Type *t, Message m) {
   if (t == GetRTTI(GameStateUpdate)) { _splashUpdate(state, m); }
   else if (t == GetRTTI(GameStateHandleInput)) { _splashHandleInput(state, m); }
   else if (t == GetRTTI(GameStateRender)) { _splashRender(state, m); }
   else if (t == GetRTTI(StateEnter)) { _splashEnter(state, m); }
   else if (t == GetRTTI(StateExit)) { _splashExit(state, m); }
}

static void _testSplashText(SplashState *self, Frame *frame) {
}

static void _registerTextRenders(SplashState *state) {
   LayerRenderer splash;
   closureInit(LayerRenderer)(&splash, state, &_testSplashText, NULL);
   renderManagerAddLayerRenderer(state->view->managers->renderManager, LayerConsole, splash);
}

void _splashEnter(SplashState *state, StateEnter *m) {
   BTManagers *managers = state->view->managers;
   byte *buffer;
   int bSize;

   _registerTextRenders(state);
   
   verbManagerSetEnabled(managers->verbManager, false);
   changeBackground(state->view, "splash");

   if (!DBSelectPalette(state->view->db, stringIntern("splash"), &buffer, &bSize)) {
      appSetPalette(appGet(), (Palette*)buffer);
   }
}
void _splashExit(SplashState *state, StateExit *m) {
   BTManagers *managers = state->view->managers;

   renderManagerRemoveLayerRenderer(state->view->managers->renderManager, LayerConsole);
   mapEditorSetEnabled(state->view->mapEditor, false);
}

void _splashUpdate(SplashState *state, GameStateUpdate *m) {
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(managers->cursorManager, mousePos.x, mousePos.y);

   if (state->pop) {
      byte *buffer;
      int bSize;

      if (!DBSelectPalette(state->view->db, stringIntern("default"), &buffer, &bSize)) {
         appSetPalette(appGet(), (Palette*)buffer);
      }

      fsmPush(state->view->gameState, gameStateCreateWorld(state->view));
   }

}

static void _handleKeyboard(SplashState *state) {
   BTManagers *managers = state->view->managers;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };


   while (keyboardPopEvent(k, &e)) {
      if (e.action == SegaKey_Released) {
         state->pop = true;
      }
   }

}

static void _handleMouse(SplashState *state) {

}

void _splashHandleInput(SplashState *state, GameStateHandleInput *m) {
   _handleKeyboard(state);
   _handleMouse(state);
}

void _splashRender(SplashState *state, GameStateRender *m) {
   renderManagerRender(state->view->managers->renderManager, m->frame);
}

StateClosure gameStateCreateSplash(WorldView *view) {
   StateClosure out;
   SplashState *state = checkedCalloc(1, sizeof(SplashState));
   state->view = view;

   _splashStateCreate(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_splash, &_splashStateDestroy);
   return out;
}