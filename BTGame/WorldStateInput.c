#include "Managers.h"
#include "SEGA/Input.h"
#include "WorldState.h"
#include "Entities/Entities.h"
#include "CoreComponents.h"
#include "GameHelpers.h"
#include "Tiles.h"
#include "GridManager.h"
#include "LightGrid.h"
#include "SEGA/App.h"
#include "Console.h"
#include "GameState.h"

void worldStateHandleKeyboardConsole(WorldState *state) {
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
            consoleSetEnabled(state->view->console, false);
            break;
         }
      }
   }
}

void worldStateHandleMouseConsole(WorldState *state) {
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

void worldStateHandleKeyboard(WorldState *state) {
   BTManagers *managers = state->view->managers;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   Viewport *vp = state->view->viewport;
   int speed = 2;
   static int toggle = 1;
   static int amb = STARTING_AMBIENT_LEVEL;

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
         case SegaKey_F1:
            renderManagerToggleFPS(state->view->managers->renderManager);
            break;
         case SegaKey_GraveAccent:
            consoleSetEnabled(state->view->console, true);
            //textBoxManagerPushText(state->view->managers->textBoxManager, stringIntern("smallbox"), "[i]meme[/i]:\nan [c=0,13]element[/c] of a [c=0,7]culture[/c] or [c=0,6]system[/c] of [c=0,5]behavior[/c] that [c=0,4]may[/c] be considered to be passed from [c=0,3]one individual[/c] to another by nongenetic means, [c=0,5]especially imitation.[/c]");
            break;
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
            appLoadPalette(appGet(), toggle ? "assets/pal/dark.pal" : "assets/pal/default.pal");
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

void worldStateHandleMouse(WorldState *state) {
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