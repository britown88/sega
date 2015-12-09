#include "WorldView.h"
#include "Managers.h"
#include "CoreComponents.h"
#include "LightGrid.h"
#include "Console.h"
#include "GameState.h"
#include "GameHelpers.h"
#include "ImageLibrary.h"

#include "Entities\Entities.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "GameClock.h"

#include "segashared\CheckedMemory.h"

typedef struct {
   WorldView *view;
   bool pop, openEditor;
}ConsoleState;

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
   BTManagers *managers = state->view->managers;

   changeBackground(state->view, IMG_BG_CONSOLE);
   consoleSetEnabled(state->view->console, true);
   gameClockPause(state->view->gameClock);

}
void _consoleExit(ConsoleState *state, StateExit *m) {
   BTManagers *managers = state->view->managers;

   consoleSetEnabled(state->view->console, false);
   gameClockResume(state->view->gameClock);
}



void _consoleUpdate(ConsoleState *state, GameStateUpdate *m) {
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(managers->cursorManager, mousePos.x, mousePos.y);
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
   BTManagers *managers = state->view->managers;
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
         //LightComponent *lc = entityGet(LightComponent)(state->mouseLight);
         //lc->radius = MAX(0, lc->radius + event.pos.y);

         consoleScrollLog(state->view->console, event.pos.y);
      }
      else if (event.action == SegaMouse_Pressed) {
         if (rectiContains(vpArea, pos)) {
            Entity *e = gridMangerEntityFromScreenPosition(state->view->managers->gridManager, pos);


            if (e && entityGet(ActorComponent)(e)) {
               consoleMacroActor(state->view->console, e);
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
   renderManagerRender(state->view->managers->renderManager, m->frame);
}

static void _enterState(ConsoleState *state) {

}

StateClosure gameStateCreateConsole(WorldView *view) {
   StateClosure out;
   ConsoleState *state = checkedCalloc(1, sizeof(ConsoleState));
   state->view = view;

   _enterState(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_console, &_consoleStateDestroy);

   return out;
}