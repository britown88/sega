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

typedef struct {
   WorldView *view;
   Entity *mouseLight;
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
   Viewport *vp = &state->view->viewport;
   int speed = 1;

   while (keyboardPopEvent(k, &e)) {
      if (e.action == SegaKey_Released && e.key == SegaKey_F1) {
         renderManagerToggleFPS(state->view->managers->renderManager);
      }

      if (e.action == SegaKey_Released && e.key == SegaKey_KeypadAdd) {
         LightComponent *lc = entityGet(LightComponent)(state->mouseLight);
         lc->centerLevel = MIN(lc->centerLevel + 1, MAX_BRIGHTNESS);
      }

      if (e.action == SegaKey_Released && e.key == SegaKey_KeypadSubtract) {
         LightComponent *lc = entityGet(LightComponent)(state->mouseLight);
         lc->centerLevel = MAX(lc->centerLevel - 1, 0);
      }

      if (e.action == SegaKey_Released && e.key == SegaKey_Escape) {
         appQuit(appGet());
      }
   }

   if (keyboardIsDown(k, SegaKey_W)) {
      vp->worldPos.y -= speed;
   }
   if (keyboardIsDown(k, SegaKey_S)) {
      vp->worldPos.y += speed;
   }
   if (keyboardIsDown(k, SegaKey_A)) {
      vp->worldPos.x -= speed;
   }
   if (keyboardIsDown(k, SegaKey_D)) {
      vp->worldPos.x += speed;
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
      if (event.action == SegaMouse_Scrolled) {
         LightComponent *lc = entityGet(LightComponent)(state->mouseLight);
         lc->radius = MAX(0, lc->radius + event.pos.y);
      }
      else if (event.action == SegaMouse_Released && event.button == SegaMouseBtn_Left) {
         LightComponent *lc = entityGet(LightComponent)(state->mouseLight);
         PositionComponent *pc = entityGet(PositionComponent)(state->mouseLight);
         int x = pc->x, y = pc->y;
         byte rad = lc->radius, cl = lc->centerLevel;
         Entity *e= entityCreate(state->view->entitySystem);
         COMPONENT_ADD(e, PositionComponent, .x = x, .y = y);
         COMPONENT_ADD(e, LightComponent, .radius = rad, .centerLevel = cl);
         entityUpdate(e);
      }
      else if (event.action == SegaMouse_Released && event.button == SegaMouseBtn_Right) {
         PositionComponent *pc = entityGet(PositionComponent)(state->mouseLight);
         int x = pc->x, y = pc->y;
         Entity *e = entityCreate(state->view->entitySystem);
         COMPONENT_ADD(e, PositionComponent, .x = x, .y = y);
         COMPONENT_ADD(e, OcclusionComponent, 0);
         entityUpdate(e);
      }
   }

   COMPONENT_LOCK(PositionComponent, cpos, state->mouseLight, {
      cpos->x = pos.x - vp->region.origin_x + vp->worldPos.x;
      cpos->y = pos.y - vp->region.origin_y + vp->worldPos.y;
   });

   //vp->worldPos.x = pos.x - vp->region.origin_x - (vp->region.width / 2);
   //vp->worldPos.y = pos.y - vp->region.origin_y - (vp->region.height / 2);
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

static void _addTestEntities(WorldState *state) {
   Entity *e = entityCreate(state->view->entitySystem);
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/grid.ega"));
   COMPONENT_ADD(e, LayerComponent, LayerBackground);
   entityUpdate(e);

   state->mouseLight = entityCreate(state->view->entitySystem);
   COMPONENT_ADD(state->mouseLight, PositionComponent, 0, 0);
   COMPONENT_ADD(state->mouseLight, LightComponent, .radius = 10, .centerLevel = MAX_BRIGHTNESS);
   entityUpdate(state->mouseLight);

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

   _addTestEntities(state);
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