#include "WorldView.h"
#include "Managers.h"
#include "SelectionManager.h"
#include "GridManager.h"
#include "Entities\Entities.h"
#include "ImageLibrary.h"
#include "segalib\EGA.h"

#include "GameState.h"
#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"

#include "Commands.h"
#include "CoreComponents.h"
#include "segashared\Strings.h"
#include "LogManager.h"

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
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(managers->cursorManager, mousePos.x, mousePos.y);
   interpolationManagerUpdate(managers->interpolationManager);
   diceManagerUpdate(managers->diceManager);

   if (!state->paused){
      commandManagerUpdate(managers->commandManager);      
   }

   logManagerUpdate(managers->logManager);
}

static void _handleKeyboard(BoardState *state){
   BTManagers *managers = state->view->managers;
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
               interpolationManagerResume(managers->interpolationManager);
            }
            else{
               state->paused = true;
               interpolationManagerPause(managers->interpolationManager);
            }
         }
         break;
      }

   }
}

static void _handleMouse(BoardState *state){
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Keyboard *k = appGetKeyboard(appGet());
   MouseEvent event = { 0 };
   while (mousePopEvent(mouse, &event)){

      switch (event.action){
      case SegaMouse_Pressed:
         if (event.button == SegaMouseBtn_Left){
            cursorManagerStartDrag(managers->cursorManager, event.pos.x, event.pos.y);
         }
         
         break;
      case SegaMouse_Released:
         if (event.button == SegaMouseBtn_Left){
            Recti mouseArea = cursorManagerEndDrag(managers->cursorManager, event.pos.x, event.pos.y);

            selectionManagerSelect(managers->selectionManager, 
               { scArea, .box = mouseArea }, 
               { scTeam, .teamID = 0 });           
         }
         else if (event.button == SegaMouseBtn_Right){
            bool shift = keyboardIsDown(k, SegaKey_LeftShift) || keyboardIsDown(k, SegaKey_RightShift);
            vec(EntityPtr) *selectedEntities = selectionManagerGetSelected(managers->selectionManager);
            
            size_t gridIndex = gridIndexFromScreenXY(event.pos.x, event.pos.y);

            logManagerPushMessage(managers->logManager, "This is a test %i", gridIndex);

            if (gridIndex < INF){
               int gx, gy;
               gridXYFromIndex(gridIndex, &gx, &gy);
               vecForEach(EntityPtr, e, selectedEntities, {
                  if (!shift){
                     entityCancelCommands(*e);
                  }
                  
                  entityPushCommand(*e, createActionGridMove(managers->commandManager, gx, gy));
               });
            }
         }
         break;
      }

      if (event.action == SegaMouse_Scrolled){
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

static void _createTestEntity(EntitySystem *system, int x, int y, bool AI){
   Entity *e = entityCreate(system);

   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern(AI ? "assets/img/badguy.ega" : "assets/img/actor.ega"));

   COMPONENT_ADD(e, LayerComponent, LayerTokens);;
   COMPONENT_ADD(e, GridComponent, .x = x, .y = y);
   COMPONENT_ADD(e, SizeComponent, 32, 32);
   COMPONENT_ADD(e, TeamComponent, AI ? 1 : 0);
   //COMPONENT_ADD(e, WanderComponent, 1);

   entityUpdate(e);
}

StateClosure gameStateCreateBoard(WorldView *view){
   StateClosure out;
   BoardState *state = checkedCalloc(1, sizeof(BoardState));

   state->view = view;
   state->paused = false;

   _createTestEntity(view->entitySystem, 0, 0, false);
   _createTestEntity(view->entitySystem, 11, 7, true);   


   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_board, &_boardStateDestroy);

   return out;
}