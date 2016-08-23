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

#include "AssetHelpers.h"

#define VP_SPEED 3
#define VP_FAST_SPEED 8

typedef struct {
   WorldView *view;
   bool pop;

   TextArea *select, *cont, *new, *ack, *quit, *copyright;
   ManagedImage *frames[10];
   ManagedImage *splash;

}SplashState;

static void _splashStateCreate(SplashState *state) {

   state->select = textAreaCreate(17, 17, 6, 1);
   state->cont = textAreaCreate(13, 19, 13, 1);
   state->new = textAreaCreate(15, 20, 10, 1);
   state->ack = textAreaCreate(12, 21, 66, 1);
   state->quit = textAreaCreate(13, 22, 13, 1);
   state->copyright = textAreaCreate(11, 24, 18, 1);
}
static void _splashStateDestroy(SplashState *self) {
   textAreaDestroy(self->select);
   textAreaDestroy(self->cont);
   textAreaDestroy(self->new);
   textAreaDestroy(self->ack);
   textAreaDestroy(self->quit);
   textAreaDestroy(self->copyright);
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
   //LayerRenderer splash;
   //closureInit(LayerRenderer)(&splash, state, &_testSplashText, NULL);
   //renderManagerAddLayerRenderer(state->view->managers->renderManager, LayerConsole, splash);
}

void _splashEnter(SplashState *state, StateEnter *m) {
   WorldView *view = state->view;

   state->splash = imageLibraryGetImage(state->view->imageLibrary, stringIntern("splash"));


   state->frames[0] = imageLibraryGetImage(view->imageLibrary, stringIntern("fire1"));
   state->frames[1] = imageLibraryGetImage(view->imageLibrary, stringIntern("fire2"));
   state->frames[2] = imageLibraryGetImage(view->imageLibrary, stringIntern("fire3"));
   state->frames[3] = imageLibraryGetImage(view->imageLibrary, stringIntern("fire4"));
   state->frames[4] = imageLibraryGetImage(view->imageLibrary, stringIntern("fire5"));
   state->frames[5] = imageLibraryGetImage(view->imageLibrary, stringIntern("fire6"));
   state->frames[6] = imageLibraryGetImage(view->imageLibrary, stringIntern("fire7"));
   state->frames[7] = imageLibraryGetImage(view->imageLibrary, stringIntern("fire8"));
   state->frames[8] = imageLibraryGetImage(view->imageLibrary, stringIntern("fire9"));
   state->frames[9] = imageLibraryGetImage(view->imageLibrary, stringIntern("fire10"));

   _registerTextRenders(state);

   textAreaSetText(state->select, "Select");
   textAreaSetText(state->cont, "[i]Journey forth[/i]");
   textAreaSetText(state->new, "Begin anew");
   textAreaSetText(state->ack, "Acknowledgements");
   textAreaSetText(state->quit, "Retire to DOS");
   textAreaSetText(state->copyright, "Copyright 1988 BDT");
   
   verbManagerSetEnabled(view->verbManager, false);
   //changeBackground(view, "splash");

   //cursorManagerSetShown(view->cursorManager, false);

   assetsSetPalette(view->db, stringIntern("splash"));
   

}
void _splashExit(SplashState *state, StateExit *m) {
   WorldView *view = state->view;
   int i = 0;
   for (i = 0; i < 10; ++i) {
      managedImageDestroy(state->frames[i]);
   }

   managedImageDestroy(state->splash);

   //cursorManagerSetShown(view->cursorManager, true);
}

void _splashUpdate(SplashState *state, GameStateUpdate *m) {
   WorldView *view = state->view;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(view->cursorManager, mousePos.x, mousePos.y);

   if (state->pop) {
      assetsSetPalette(state->view->db, stringIntern("default"));

      fsmPush(state->view->gameState, gameStateCreateWorld(state->view));

      luaStartup(state->view->L);
   }

}

static void _handleKeyboard(SplashState *state) {
   WorldView *view = state->view;
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
   Milliseconds t = t_u2m(gameClockGetTime(state->view->gameClock));
   int f = (int)(t / 200.0f) % 10;
   Frame *frame = m->frame;

   frameClear(frame, FrameRegionFULL, 0);


   frameRenderImage(frame, FrameRegionFULL, 0, 0, managedImageGetImage(state->splash));
   frameRenderImage(frame, FrameRegionFULL, 79, 80, managedImageGetImage(state->frames[f]));

   textAreaRender(state->select, state->view, frame);
   textAreaRender(state->cont, state->view, frame);
   textAreaRender(state->new, state->view, frame);
   textAreaRender(state->ack, state->view, frame);
   textAreaRender(state->quit, state->view, frame);
   textAreaRender(state->copyright, state->view, frame);

}

StateClosure gameStateCreateSplash(WorldView *view) {
   StateClosure out;
   SplashState *state = checkedCalloc(1, sizeof(SplashState));
   state->view = view;

   _splashStateCreate(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_splash, &_splashStateDestroy);
   return out;
}