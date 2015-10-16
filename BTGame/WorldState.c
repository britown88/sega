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

#include "segautils/Lisp.h"

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
      if (e.action == SegaKey_Pressed) {
         switch (e.key) {
         case SegaKey_Up:
            state->view->viewport.worldPos.y -= 2;
            break;
         case SegaKey_Down:
            state->view->viewport.worldPos.y += 2;
            break;
         case SegaKey_Left:
            state->view->viewport.worldPos.x -= 2;
            break;
         case SegaKey_Right:
            state->view->viewport.worldPos.x += 2;
            break;
         }
      }
   }
}

static void _handleMouse(WorldState *state){
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Keyboard *k = appGetKeyboard(appGet());
   MouseEvent event = { 0 };
   Int2 pos = mouseGetPosition(mouse);
   Viewport *vp = &state->view->viewport;
   while (mousePopEvent(mouse, &event)){
   }

   vp->worldPos.x = pos.x - vp->region.origin_x - (vp->region.width / 2);
   vp->worldPos.y = pos.y - vp->region.origin_y - (vp->region.height / 2);
}

void _boardHandleInput(WorldState *state, GameStateHandleInput *m){
   _handleKeyboard(state);
   _handleMouse(state);
}

void _boardRender(WorldState *state, GameStateRender *m){
   renderManagerRender(state->view->managers->renderManager, m->frame);
}

static void _testLisp() {
   LispExpr ex = lispCreatef32(1.0f);

   float *f = lispf32(&ex);
   int *i = lispi32(&ex);
}

static void _addTestEntities(WorldView *view) {
   Entity *e = entityCreate(view->entitySystem);
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/grid.ega"));
   COMPONENT_ADD(e, LayerComponent, LayerBackground);
   entityUpdate(e);

   //e = entityCreate(view->entitySystem);
   //COMPONENT_ADD(e, PositionComponent, 0, 0);
   //COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/dotagrid.ega"));
   //COMPONENT_ADD(e, InViewComponent, 0);
   //COMPONENT_ADD(e, LayerComponent, LayerGrid);
   //entityUpdate(e);
}

static void _enterState(WorldState *state) {
   appLoadPalette(appGet(), "assets/img/default.pal");
   cursorManagerCreateCursor(state->view->managers->cursorManager);

   _addTestEntities(state->view);
   _testLisp();
}

StateClosure gameStateCreateWorld(WorldView *view){
   StateClosure out;
   WorldState *state = checkedCalloc(1, sizeof(WorldState));
   state->view = view;

   _enterState(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_board, &_boardStateDestroy);

   return out;
}