#include "WorldView.h"
#include "Managers.h"
#include "Entities\Entities.h"
#include "ImageLibrary.h"
#include "segalib\EGA.h"

#include "GameState.h"
#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "BT.h"

static void _boardUpdate(WorldView*, GameStateUpdate*);
static void _boardHandleInput(WorldView*, GameStateHandleInput*);
static void _boardRender(WorldView*, GameStateRender*);

static void _board(WorldView *view, Type *t, Message m){
   if (t == GetRTTI(GameStateUpdate)){ _boardUpdate(view, m); }
   else if (t == GetRTTI(GameStateHandleInput)){ _boardHandleInput(view, m); }
   else if (t == GetRTTI(GameStateRender)){ _boardRender(view, m); }
}

void _boardUpdate(WorldView *view, GameStateUpdate *m){
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);
   cursorManagerUpdate(view->managers->cursorManager, mousePos.x, mousePos.y);

   interpolationManagerUpdate(view->managers->interpolationManager);
   derjpkstras(view->entitySystem, view->managers->gridManager);

   diceManagerUpdate(view->managers->diceManager);
}

static void _handleKeyboard(WorldView *view){

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
            interpolationManagerPause(view->managers->interpolationManager);
            fsmPush(view->gameState, gameStateCreateBoardPaused(view));
         }
         break;
      }

   }
}

static void _handleMouse(){
   Mouse *k = appGetMouse(appGet());
   MouseEvent e = { 0 };
   while (mousePopEvent(k, &e)){
      if (e.action == SegaMouse_Scrolled){
         //size += e.pos.y;
      }
   }
}

void _boardHandleInput(WorldView *view, GameStateHandleInput *m){
   _handleKeyboard(view);
   _handleMouse();
}

void _boardRender(WorldView *view, GameStateRender *m){
   renderManagerRender(view->managers->renderManager, m->frame);
}

StateClosure gameStateCreateBoard(WorldView *view){
   StateClosure out;
   closureInit(StateClosure)(&out, view, (StateClosureFunc)&_board, NULL);
   return out;
}