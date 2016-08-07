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
#include "Lua.h"

#include "Entities\Entities.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "GameClock.h"
#include "TextArea.h"

#include "segashared\CheckedMemory.h"

#include "ImageLibrary.h"

#define VP_SPEED 3
#define VP_FAST_SPEED 8

typedef struct {
   WorldView *view;
   bool pop;

   TextArea *select, *cont, *new, *ack, *quit, *copyright;
   ManagedImage *frames[10];

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

   Milliseconds t = t_u2m(gameClockGetTime(self->view->gameClock));
   int f = (int)(t / 200.0f) % 10;

   frameRenderImage(frame, FrameRegionFULL, 79, 80, managedImageGetImage(self->frames[f]));

   textAreaRender(self->select, self->view, frame);
   textAreaRender(self->cont, self->view, frame);
   textAreaRender(self->new, self->view, frame);
   textAreaRender(self->ack, self->view, frame);
   textAreaRender(self->quit, self->view, frame);
   textAreaRender(self->copyright, self->view, frame);
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

   state->frames[0] = imageLibraryGetImage(state->view->imageLibrary, stringIntern("fire1"));
   state->frames[1] = imageLibraryGetImage(state->view->imageLibrary, stringIntern("fire2"));
   state->frames[2] = imageLibraryGetImage(state->view->imageLibrary, stringIntern("fire3"));
   state->frames[3] = imageLibraryGetImage(state->view->imageLibrary, stringIntern("fire4"));
   state->frames[4] = imageLibraryGetImage(state->view->imageLibrary, stringIntern("fire5"));
   state->frames[5] = imageLibraryGetImage(state->view->imageLibrary, stringIntern("fire6"));
   state->frames[6] = imageLibraryGetImage(state->view->imageLibrary, stringIntern("fire7"));
   state->frames[7] = imageLibraryGetImage(state->view->imageLibrary, stringIntern("fire8"));
   state->frames[8] = imageLibraryGetImage(state->view->imageLibrary, stringIntern("fire9"));
   state->frames[9] = imageLibraryGetImage(state->view->imageLibrary, stringIntern("fire10"));

   _registerTextRenders(state);

   textAreaPushText(state->select, "Select"); textAreaUpdate(state->select, state->view);
   textAreaPushText(state->cont, "[i]Journey forth[/i]"); textAreaUpdate(state->cont, state->view);
   textAreaPushText(state->new, "Begin anew"); textAreaUpdate(state->new, state->view);
   textAreaPushText(state->ack, "Acknowledgements"); textAreaUpdate(state->ack, state->view);
   textAreaPushText(state->quit, "Retire to DOS"); textAreaUpdate(state->quit, state->view);
   textAreaPushText(state->copyright, "Copyright 1988 BDT"); textAreaUpdate(state->copyright, state->view);
   
   verbManagerSetEnabled(managers->verbManager, false);
   changeBackground(state->view, "splash");

   cursorManagerSetShown(state->view->managers->cursorManager, false);

   if (!DBSelectPalette(state->view->db, stringIntern("splash"), &buffer, &bSize)) {
      appSetPalette(appGet(), (Palette*)buffer);
   }
}
void _splashExit(SplashState *state, StateExit *m) {
   BTManagers *managers = state->view->managers;
   int i = 0;
   for (i = 0; i < 10; ++i) {
      managedImageDestroy(state->frames[i]);
   }

   cursorManagerSetShown(state->view->managers->cursorManager, true);
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

      luaStartup(state->view->L);
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