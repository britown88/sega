#include "WorldView.h"
#include "Managers.h"
#include "SelectionManager.h"
#include "Entities\Entities.h"
#include "ImageLibrary.h"
#include "segalib\EGA.h"

#include "GameState.h"
#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"
#include "BT.h"

typedef struct {
   WorldView *view;
   bool paused;
}BoardState;

static void _boardStateDestroy(BoardState *self){
   checkedFree(self);
}

static void _boardUpdate(BoardState*, GameStateUpdate*);
static void _boardHandleInput(BoardState*, GameStateHandleInput*);
static void _boardRender(BoardState*, GameStateRender*);

static void _board(BoardState *state, Type *t, Message m){
   if (t == GetRTTI(GameStateUpdate)){ _boardUpdate(state, m); }
   else if (t == GetRTTI(GameStateHandleInput)){ _boardHandleInput(state, m); }
   else if (t == GetRTTI(GameStateRender)){ _boardRender(state, m); }
}

void _boardUpdate(BoardState *state, GameStateUpdate *m){
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);
   cursorManagerUpdate(state->view->managers->cursorManager, mousePos.x, mousePos.y);

   interpolationManagerUpdate(state->view->managers->interpolationManager);
   if (!state->paused){
      derjpkstras(state->view->entitySystem, state->view->managers->gridManager);
   }   

   diceManagerUpdate(state->view->managers->diceManager);
}

static void _handleKeyboard(BoardState *state){

   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   while (keyboardPopEvent(k, &e)){
      switch (e.key){
      case (SegaKey_Escape) :
         if (e.action == SegaKey_Released){
            appQuit(appGet());
         }
         break;
      case (SegaKey_Space) :
         if (e.action == SegaKey_Released){
            if (state->paused){
               state->paused = false;
               interpolationManagerResume(state->view->managers->interpolationManager);
            }
            else{
               state->paused = true;
               interpolationManagerPause(state->view->managers->interpolationManager);
            }
         }
         break;
      }

   }
}

static void _handleMouse(BoardState *state){
   BTManagers *managers = state->view->managers;
   Mouse *k = appGetMouse(appGet());
   MouseEvent e = { 0 };
   while (mousePopEvent(k, &e)){

      switch (e.action){
      case SegaMouse_Pressed:
         if (e.button == SegaMouseBtn_Left){
            cursorManagerStartDrag(managers->cursorManager, e.pos.x, e.pos.y);
         }
         
         break;
      case SegaMouse_Released:
         if (e.button == SegaMouseBtn_Left){
            Recti mouseArea = cursorManagerEndDrag(managers->cursorManager, e.pos.x, e.pos.y);

            selectionManagerSelect(managers->selectionManager, 
               { scArea, .box = mouseArea }, 
               { scTeam, .teamID = 1 });
            
         }
         
         break;
      }

      if (e.action == SegaMouse_Scrolled){
         //size += e.pos.y;
      }

   }
}

void _boardHandleInput(BoardState *state, GameStateHandleInput *m){
   _handleKeyboard(state);
   _handleMouse(state);
}

void _boardRender(BoardState *state, GameStateRender *m){

   renderManagerRender(state->view->managers->renderManager, m->frame);
   
}

StateClosure gameStateCreateBoard(WorldView *view){
   StateClosure out;
   BoardState *state = checkedCalloc(1, sizeof(BoardState));
   state->view = view;
   state->paused = false;

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_board, &_boardStateDestroy);
   return out;
}