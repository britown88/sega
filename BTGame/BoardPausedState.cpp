#include "WorldView.h"
#include "Managers.h"
#include "Entities\Entities.h"
#include "ImageLibrary.h"
#include "segalib\EGA.h"

#include "GameState.h"
#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "BT.h"

static void _bpausedUpdate(WorldView*, GameStateUpdate*);
static void _bpausedHandleInput(WorldView*, GameStateHandleInput*);
static void _bpausedRender(WorldView*, GameStateRender*);

static void _bpaused(WorldView *view, Type *t, Message m){
   if (t == GetRTTI(GameStateUpdate)){ _bpausedUpdate(view, m); }
   else if (t == GetRTTI(GameStateHandleInput)){ _bpausedHandleInput(view, m); }
   else if (t == GetRTTI(GameStateRender)){ _bpausedRender(view, m); }
}

void _bpausedUpdate(WorldView *view, GameStateUpdate *m){
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);
   cursorManagerUpdate(view->managers->cursorManager, mousePos.x, mousePos.y);

   interpolationManagerUpdate(view->managers->interpolationManager);

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
            interpolationManagerResume(view->managers->interpolationManager);
            fsmPop(view->gameState);
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

void _bpausedHandleInput(WorldView *view, GameStateHandleInput *m){
   _handleKeyboard(view);
   _handleMouse();
}

void _bpausedRender(WorldView *view, GameStateRender *m){
   renderManagerRender(view->managers->renderManager, m->frame);
}

StateClosure gameStateCreateBoardPaused(WorldView *view){
   StateClosure out;
   closureInit(StateClosure)(&out, view, (StateClosureFunc)&_bpaused, NULL);
   return out;
}