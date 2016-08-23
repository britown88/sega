#include "WorldView.h"
#include "Managers.h"
#include "LightGrid.h"
#include "Console.h"
#include "GameState.h"
#include "GameHelpers.h"
#include "ImageLibrary.h"
#include "Weather.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "GameClock.h"

#include "segashared\CheckedMemory.h"

typedef struct {
   WorldView *view;
   bool pop, openEditor;
   ManagedImage *bg;
}ConsoleState;

static void _consoleStateCreate(ConsoleState *state) {}
static void _consoleStateDestroy(ConsoleState *self) {
   checkedFree(self);
}

static void _consoleUpdate(ConsoleState*, GameStateUpdate*);
static void _consoleHandleInput(ConsoleState*, GameStateHandleInput*);
static void _consoleRender(ConsoleState*, GameStateRender*);
static void _consoleEnter(ConsoleState*, StateEnter*);
static void _consoleExit(ConsoleState*, StateExit*);
static void _consoleOpenEditor(ConsoleState*, GameStateOpenMapEditor*);

static void _console(ConsoleState *state, Type *t, Message m) {
   if (t == GetRTTI(GameStateUpdate)) { _consoleUpdate(state, m); }
   else if (t == GetRTTI(GameStateHandleInput)) { _consoleHandleInput(state, m); }
   else if (t == GetRTTI(GameStateRender)) { _consoleRender(state, m); }
   else if (t == GetRTTI(StateEnter)) { _consoleEnter(state, m); }
   else if (t == GetRTTI(StateExit)) { _consoleExit(state, m); }
   else if (t == GetRTTI(GameStateOpenMapEditor)) { _consoleOpenEditor(state, m); }
}

void _consoleOpenEditor(ConsoleState *state, GameStateOpenMapEditor *m) {
   state->pop = true;
   state->openEditor = true;
}

void _consoleEnter(ConsoleState *state, StateEnter *m) {

   state->bg = imageLibraryGetImage(state->view->imageLibrary, stringIntern(IMG_BG_CONSOLE));
   consoleSetEnabled(state->view->console, true);
   gameClockPause(state->view->gameClock);

}
void _consoleExit(ConsoleState *state, StateExit *m) {

   managedImageDestroy(state->bg);
   consoleSetEnabled(state->view->console, false);
   gameClockResume(state->view->gameClock);
}



void _consoleUpdate(ConsoleState *state, GameStateUpdate *m) {

   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(state->view->cursorManager, mousePos.x, mousePos.y);
   consoleUpdate(state->view->console);

   if (state->pop) {
      bool openEditor = state->openEditor;
      FSM *fsm = state->view->gameState;
      fsmPop(fsm);

      if (openEditor) {
         fsmSend(fsm, GameStateOpenMapEditor);
      }
   }
}

static void _handleKeyboard(ConsoleState *state) {
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };

   while (keyboardPopEvent(k, &e)) {
      if (e.action == SegaKey_Char) {
         if (e.unichar < 256 && e.unichar != '`' && e.unichar != '~') {
            consoleInputChar(state->view->console, (char)e.unichar);
         }
      }
      else if (e.action == SegaKey_Pressed || e.action == SegaKey_Repeat) {
         consoleInputKey(state->view->console, e.key, e.mods);
      }
      else if (e.action == SegaKey_Released) {
         switch (e.key) {
         case SegaKey_Escape:
            appQuit(appGet());
            break;
         case SegaKey_GraveAccent:
            state->pop = true;
            break;
         }
      }
   }
}

static void _handleMouse(ConsoleState *state) {
   Mouse *mouse = appGetMouse(appGet());
   MouseEvent event = { 0 };
   Int2 pos = mouseGetPosition(mouse);
   Viewport *vp = state->view->viewport;
   Recti vpArea = { vp->region.origin_x, vp->region.origin_y, vp->region.origin_x + vp->region.width, vp->region.origin_y + vp->region.height };

   while (mousePopEvent(mouse, &event)) {
      if (event.action == SegaMouse_Scrolled) {
         consoleScrollLog(state->view->console, event.pos.y);
      }
      else if (event.action == SegaMouse_Pressed) {
         if (rectiContains(vpArea, pos)) {
            Actor *a = gridManagerActorFromScreenPosition(state->view->gridManager, pos);
            if (a) {
               consoleMacroActor(state->view->console, a);
            }
            else {
               Int2 worldPos = screenToWorld(state->view, pos);
               consoleMacroGridPos(state->view->console,
                  worldPos.x / GRID_CELL_SIZE,
                  worldPos.y / GRID_CELL_SIZE);
            }
         }
      }
   }
}

void _consoleHandleInput(ConsoleState *state, GameStateHandleInput *m) {
   _handleKeyboard(state);
   _handleMouse(state);   
}

void _consoleRender(ConsoleState *state, GameStateRender *m) {
   Frame *frame = m->frame;
   frameClear(frame, FrameRegionFULL, 0);

   frameRenderImage(m->frame, FrameRegionFULL, 0, 0, managedImageGetImage(state->bg));

   gridManagerRender(state->view->gridManager, frame);
   actorManagerRender(state->view->actorManager, frame);
   weatherRender(state->view->weather, frame);
   gridManagerRenderLighting(state->view->gridManager, frame);

   cursorManagerRender(state->view->cursorManager, frame);

   framerateViewerRender(state->view->framerateViewer, frame);
   consoleRenderLines(state->view->console, frame);
}

StateClosure gameStateCreateConsole(WorldView *view) {
   StateClosure out;
   ConsoleState *state = checkedCalloc(1, sizeof(ConsoleState));
   state->view = view;

   _consoleStateCreate(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_console, &_consoleStateDestroy);

   return out;
}