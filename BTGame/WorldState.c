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
#include "LightGrid.h"

#include "segautils/Lisp.h"
#include "segautils/Math.h"

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
   interpolationManagerUpdate(managers->interpolationManager);
   gridMovementManagerUpdate(managers->gridMovementManager);
   pcManagerUpdate(managers->pcManager);
}

static void _handleKeyboard(WorldState *state){
   BTManagers *managers = state->view->managers;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   Viewport *vp = state->view->viewport;
   int speed = 2;
   static int toggle = 1;
   static int amb = 0;

   while (keyboardPopEvent(k, &e)) {
      if (e.action == SegaKey_Released && e.key == SegaKey_F1) {
         renderManagerToggleFPS(state->view->managers->renderManager);
      }

      if (e.action == SegaKey_Pressed) {
         switch (e.key) {
         case SegaKey_LeftControl:
            pcManagerSetSneak(state->view->managers->pcManager, true);
            break;
         }
      }
      else if (e.action == SegaKey_Released) {
         switch (e.key) {
         case SegaKey_LeftControl:
            pcManagerSetSneak(state->view->managers->pcManager, false);
            break;
         case SegaKey_KeypadAdd:
            amb = MIN(amb + 1, MAX_BRIGHTNESS);
            gridManagerSetAmbientLight(state->view->managers->gridManager, amb);
            break;
         case SegaKey_KeypadSubtract:
            amb = MAX(amb - 1, 0);
            gridManagerSetAmbientLight(state->view->managers->gridManager, amb);
            break;
         case SegaKey_Escape:
            appQuit(appGet());
            break;
         case SegaKey_P:
            appLoadPalette(appGet(), toggle ? "assets/img/dark2.pal" : "assets/img/default2.pal");
            toggle = !toggle;
            break;
         case SegaKey_T:
            pcManagerToggleTorch(state->view->managers->pcManager);
            break;
         case SegaKey_W:
         case SegaKey_A:
         case SegaKey_S:
         case SegaKey_D:
            pcManagerStop(state->view->managers->pcManager);
            break;
         }
      }

   }

   if (keyboardIsDown(k, SegaKey_W)) {
      pcManagerMoveRelative(state->view->managers->pcManager, 0, -1);
   }
   if (keyboardIsDown(k, SegaKey_S)) {
      pcManagerMoveRelative(state->view->managers->pcManager, 0, 1);
   }
   if (keyboardIsDown(k, SegaKey_A)) {
      pcManagerMoveRelative(state->view->managers->pcManager, -1, 0);
   }
   if (keyboardIsDown(k, SegaKey_D)) {
      pcManagerMoveRelative(state->view->managers->pcManager, 1, 0);
   }
}

static void _handleMouse(WorldState *state){
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Keyboard *k = appGetKeyboard(appGet());
   MouseEvent event = { 0 };
   Int2 pos = mouseGetPosition(mouse);
   Viewport *vp = state->view->viewport;
   while (mousePopEvent(mouse, &event)){
      if (event.action == SegaMouse_Scrolled) {
         //LightComponent *lc = entityGet(LightComponent)(state->mouseLight);
         //lc->radius = MAX(0, lc->radius + event.pos.y);
      }
      else if (event.action == SegaMouse_Released && event.button == SegaMouseBtn_Left) {
         int x = (pos.x - vp->region.origin_x + vp->worldPos.x) / GRID_CELL_SIZE;
         int y = (pos.y - vp->region.origin_y + vp->worldPos.y) / GRID_CELL_SIZE;
         Tile *t = gridManagerTileAtXY(state->view->managers->gridManager, x, y);
         if (t) {
            t->schema = 7;
            t->collision = GRID_SOLID;
         }
      }

   }

   if (mouseIsDown(mouse, SegaMouseBtn_Right)) {

      pcManagerMove(state->view->managers->pcManager,
         (pos.x - vp->region.origin_x + vp->worldPos.x) / GRID_CELL_SIZE,
         (pos.y - vp->region.origin_y + vp->worldPos.y) / GRID_CELL_SIZE);
   }

}

void _boardHandleInput(WorldState *state, GameStateHandleInput *m){
   _handleKeyboard(state);
   _handleMouse(state);
}

void _boardRender(WorldState *state, GameStateRender *m){
   renderManagerRender(state->view->managers->renderManager, m->frame);
}

static void _addTestEntities(WorldState *state) {
   Entity *e = entityCreate(state->view->entitySystem);
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/grid.ega"));
   COMPONENT_ADD(e, LayerComponent, LayerBackground);
   COMPONENT_ADD(e, RenderedUIComponent, 0);
   entityUpdate(e);


   e = entityCreate(state->view->entitySystem);
   COMPONENT_ADD(e, LayerComponent, LayerUI);
   COMPONENT_ADD(e, RenderedUIComponent, 0);
   COMPONENT_ADD(e, TextComponent, 
      .bg = 0, .fg = 15, .lines = vecInitStackArray(TextLine, { 
         { 0, 0, stringCreate("hello") },
         { 5, 1, stringCreate("world") },
         { 10, 2, stringCreate("thing") },
         { 15, 3, stringCreate("yes") }
      }));
   entityUpdate(e);
}

static void _enterState(WorldState *state) {
   appLoadPalette(appGet(), "assets/img/default2.pal");
   cursorManagerCreateCursor(state->view->managers->cursorManager);
   pcManagerCreatePC(state->view->managers->pcManager);

   _addTestEntities(state);
}

StateClosure gameStateCreateWorld(WorldView *view){
   StateClosure out;
   WorldState *state = checkedCalloc(1, sizeof(WorldState));
   state->view = view;

   _enterState(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_board, &_boardStateDestroy);

   return out;
}