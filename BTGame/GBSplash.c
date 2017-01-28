#include "WorldView.h"
#include "Managers.h"
#include "LightGrid.h"
#include "Console.h"
#include "GameState.h"
#include "GameHelpers.h"
#include "ImageLibrary.h"
#include "MapEditor.h"
#include "DB.h"
#include "Lua.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "GameClock.h"
#include "TextArea.h"

#include "segashared\CheckedMemory.h"

#include "ImageLibrary.h"
#include "Sprites.h"
#include "AssetHelpers.h"

typedef struct {
   WorldView *view;

}SplashState;

static void _splashStateCreate(SplashState *state) {

   
}
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

void _splashEnter(SplashState *state, StateEnter *m) {
   WorldView *view = state->view;


}
void _splashExit(SplashState *state, StateExit *m) {
   WorldView *view = state->view;

}

void _splashUpdate(SplashState *state, GameStateUpdate *m) {
   WorldView *view = state->view;

}

static void _handleKeyboard(SplashState *state) {
   WorldView *view = state->view;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };

   while (keyboardPopEvent(k, &e)) {     
      if (e.action == SegaKey_Released) {
         switch (e.key) {
         case SegaKey_Escape:
            appQuit(appGet());
            break;
         }
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
   Texture *frame = m->frame;


}

StateClosure gameStateCreateGBSplash(WorldView *view) {
   StateClosure out;
   SplashState *state = checkedCalloc(1, sizeof(SplashState));
   state->view = view;

   _splashStateCreate(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_splash, &_splashStateDestroy);
   return out;
}