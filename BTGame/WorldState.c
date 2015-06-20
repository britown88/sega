#include "WorldView.h"
#include "Managers.h"
#include "Entities\Entities.h"
#include "ImageLibrary.h"
#include "segalib\EGA.h"

#include "GameState.h"
#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"

#include "CoreComponents.h"
#include "segashared\Strings.h"
#include "GameClock.h"

typedef struct {
   WorldView *view;
}WorldState;

static void _boardStateDestroy(WorldState *self){
   checkedFree(self);
}

static void _boardUpdate(WorldState*, GameStateUpdate*);
static void _boardHandleInput(WorldState*, GameStateHandleInput*);
static void _boardRender(WorldState*, GameStateRender*);

static void _board(WorldState *state, Type *t, Message m){
   if (t == GetRTTI(GameStateUpdate)){ _boardUpdate(state, m); }
   else if (t == GetRTTI(GameStateHandleInput)){ _boardHandleInput(state, m); }
   else if (t == GetRTTI(GameStateRender)){ _boardRender(state, m); }
}


void _boardUpdate(WorldState *state, GameStateUpdate *m){
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(managers->cursorManager, mousePos.x, mousePos.y);
}

static void _handleKeyboard(WorldState *state){
   BTManagers *managers = state->view->managers;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   while (keyboardPopEvent(k, &e)){
   }
}

static void _handleMouse(WorldState *state){
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Keyboard *k = appGetKeyboard(appGet());
   MouseEvent event = { 0 };
   while (mousePopEvent(mouse, &event)){
   }
}

void _boardHandleInput(WorldState *state, GameStateHandleInput *m){
   _handleKeyboard(state);
   _handleMouse(state);
}

void _boardRender(WorldState *state, GameStateRender *m){
   renderManagerRender(state->view->managers->renderManager, m->frame);
}

StateClosure gameStateCreateWorld(WorldView *view){
   StateClosure out;
   WorldState *state = checkedCalloc(1, sizeof(WorldState));

   state->view = view;
   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_board, &_boardStateDestroy);

   return out;
}