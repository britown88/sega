#include "WorldView.h"
#include "Managers.h"
#include "CoreComponents.h"
#include "LightGrid.h"
#include "Console.h"
#include "GameState.h"
#include "GameHelpers.h"

#include "Entities\Entities.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"

#include "segashared\CheckedMemory.h"

#define STARTING_AMBIENT_LEVEL MAX_BRIGHTNESS

typedef struct {
   WorldView *view;
}WorldState;

static void _worldStateDestroy(WorldState *self){   
   checkedFree(self);
}

static void _worldUpdate(WorldState*, GameStateUpdate*);
static void _worldHandleInput(WorldState*, GameStateHandleInput*);
static void _worldRender(WorldState*, GameStateRender*);
static void _worldEnter(WorldState*, StateEnter*);
static void _worldExit(WorldState*, StateExit*);
static void _worldOpenEditor(WorldState*, GameStateOpenMapEditor*);


static void _world(WorldState *state, Type *t, Message m){
   if (t == GetRTTI(GameStateUpdate)){ _worldUpdate(state, m); }
   else if (t == GetRTTI(GameStateHandleInput)){ _worldHandleInput(state, m); }
   else if (t == GetRTTI(GameStateRender)){ _worldRender(state, m); }
   else if (t == GetRTTI(StateEnter)) { _worldEnter(state, m); }
   else if (t == GetRTTI(StateExit)) { _worldExit(state, m); }
   else if (t == GetRTTI(GameStateOpenMapEditor)) { _worldOpenEditor(state, m); }
}

void _worldOpenEditor(WorldState *state, GameStateOpenMapEditor *m) {
   fsmPush(state->view->gameState, gameStateCreateEditor(state->view));
}

void _worldUpdate(WorldState *state, GameStateUpdate *m){
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(managers->cursorManager, mousePos.x, mousePos.y);
   interpolationManagerUpdate(managers->interpolationManager);
   waitManagerUpdate(managers->waitManager);
   gridMovementManagerUpdate(managers->gridMovementManager);
   pcManagerUpdate(managers->pcManager);
   textBoxManagerUpdate(managers->textBoxManager);
   actorManagerUpdate(managers->actorManager);
}

void _registerGridRenders(BTManagers *managers) {
   LayerRenderer grid, light;

   closureInit(LayerRenderer)(&grid, managers->gridManager, &gridManagerRender, NULL);
   closureInit(LayerRenderer)(&light, managers->gridManager, &gridManagerRenderLighting, NULL);

   renderManagerAddLayerRenderer(managers->renderManager, LayerGrid, grid);
   renderManagerAddLayerRenderer(managers->renderManager, LayerGridLighting, light);
}

void _worldEnter(WorldState *state, StateEnter *m) {
   BTManagers *managers = state->view->managers;
   textBoxManagerShowTextArea(managers->textBoxManager, stringIntern("smallbox"));
   verbManagerSetEnabled(managers->verbManager, true);
   changeBackground(state->view, "assets/img/bg.ega");

   _registerGridRenders(managers);   
}
void _worldExit(WorldState *state, StateExit *m) {
   BTManagers *managers = state->view->managers;
   textBoxManagerHideTextArea(managers->textBoxManager, stringIntern("smallbox"));
   verbManagerSetEnabled(managers->verbManager, false);
}

static void _handleKeyboard(WorldState *state) {
   BTManagers *managers = state->view->managers;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   Viewport *vp = state->view->viewport;
   int speed = 2;

   while (keyboardPopEvent(k, &e)) {

      if (e.action == SegaKey_Pressed) {
         switch (e.key) {
         case SegaKey_LeftControl:
            pcManagerSetSneak(state->view->managers->pcManager, true);
            break;
         case SegaKey_1:
            verbManagerKeyButton(state->view->managers->verbManager, Verb_Look, e.action);
            break;
         case SegaKey_2:
            verbManagerKeyButton(state->view->managers->verbManager, Verb_Use, e.action);
            break;
         case SegaKey_3:
            verbManagerKeyButton(state->view->managers->verbManager, Verb_Talk, e.action);
            break;
         case SegaKey_4:
            verbManagerKeyButton(state->view->managers->verbManager, Verb_Fight, e.action);
            break;
         }
      }
      else if (e.action == SegaKey_Released) {
         switch (e.key) {
         case SegaKey_GraveAccent:
            fsmPush(state->view->gameState, gameStateCreateConsole(state->view));
            //consoleSetEnabled(state->view->console, true);
            //textBoxManagerPushText(state->view->managers->textBoxManager, stringIntern("smallbox"), "[i]meme[/i]:\nan [c=0,13]element[/c] of a [c=0,7]culture[/c] or [c=0,6]system[/c] of [c=0,5]behavior[/c] that [c=0,4]may[/c] be considered to be passed from [c=0,3]one individual[/c] to another by nongenetic means, [c=0,5]especially imitation.[/c]");
            break;
         case SegaKey_LeftControl:
            pcManagerSetSneak(state->view->managers->pcManager, false);
            break;
         case SegaKey_Escape:
            appQuit(appGet());
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
         case SegaKey_1:
            verbManagerKeyButton(state->view->managers->verbManager, Verb_Look, e.action);
            break;
         case SegaKey_2:
            verbManagerKeyButton(state->view->managers->verbManager, Verb_Use, e.action);
            break;
         case SegaKey_3:
            verbManagerKeyButton(state->view->managers->verbManager, Verb_Talk, e.action);
            break;
         case SegaKey_4:
            verbManagerKeyButton(state->view->managers->verbManager, Verb_Fight, e.action);
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

static void _dropTestBlock(WorldState *state, Int2 pos) {
   Viewport *vp = state->view->viewport;
   int x = (pos.x - vp->region.origin_x + vp->worldPos.x) / GRID_CELL_SIZE;
   int y = (pos.y - vp->region.origin_y + vp->worldPos.y) / GRID_CELL_SIZE;
   Tile *t = gridManagerTileAtXY(state->view->managers->gridManager, x, y);
   if (t) {
      t->schema = 7;
      t->collision = COL_SOLID;
   }
}

static void _handleMouse(WorldState *state) {
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Keyboard *k = appGetKeyboard(appGet());
   MouseEvent event = { 0 };
   Int2 pos = mouseGetPosition(mouse);
   Viewport *vp = state->view->viewport;
   Recti vpArea = { vp->region.origin_x, vp->region.origin_y, vp->region.origin_x + vp->region.width, vp->region.origin_y + vp->region.height };

   while (mousePopEvent(mouse, &event)) {
      if (event.action == SegaMouse_Scrolled) {
         //LightComponent *lc = entityGet(LightComponent)(state->mouseLight);
         //lc->radius = MAX(0, lc->radius + event.pos.y);
      }
      else if (event.action == SegaMouse_Pressed) {
         switch (event.button) {
         case SegaMouseBtn_Left:
            if (verbManagerMouseButton(state->view->managers->verbManager, &event)) {
               continue;
            }
            break;
         }
      }
      else if (event.action == SegaMouse_Released) {
         switch (event.button) {
         case SegaMouseBtn_Left:

            if (verbManagerMouseButton(state->view->managers->verbManager, &event)) {
               continue;
            }
            break;
         case SegaMouseBtn_Right:

            break;
         }
      }

   }

   if (mouseIsDown(mouse, SegaMouseBtn_Left)) {
      if (rectiContains(vpArea, pos)) {
         _dropTestBlock(state, pos);
      }
   }

   if (mouseIsDown(mouse, SegaMouseBtn_Right)) {
      if (rectiContains(vpArea, pos)) {
         pcManagerMove(state->view->managers->pcManager,
            (pos.x - vp->region.origin_x + vp->worldPos.x) / GRID_CELL_SIZE,
            (pos.y - vp->region.origin_y + vp->worldPos.y) / GRID_CELL_SIZE);
      }
   }
}

void _worldHandleInput(WorldState *state, GameStateHandleInput *m){
   _handleKeyboard(state);
   _handleMouse(state);  
}

void _worldRender(WorldState *state, GameStateRender *m){
   renderManagerRender(state->view->managers->renderManager, m->frame);
}

StateClosure gameStateCreateWorld(WorldView *view){
   StateClosure out;
   WorldState *state = checkedCalloc(1, sizeof(WorldState));
   state->view = view;

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_world, &_worldStateDestroy);

   return out;
}