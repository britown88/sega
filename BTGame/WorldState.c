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
#include "Verbs.h"
#include "Console.h"

#include "RichText.h"
#include "GameHelpers.h"
#include "Lua.h"

#include "segautils/Lisp.h"
#include "segautils/Math.h"

#include <stdio.h>

#define STARTING_AMBIENT_LEVEL MAX_BRIGHTNESS

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
   waitManagerUpdate(managers->waitManager);
   gridMovementManagerUpdate(managers->gridMovementManager);
   pcManagerUpdate(managers->pcManager);
   textBoxManagerUpdate(managers->textBoxManager);
   actorManagerUpdate(managers->actorManager);

   if (consoleGetEnabled(state->view->console)) {
      consoleUpdate(state->view->console);
   }
}

static void _handleKeyboardConsole(WorldState *state) {
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

static void _handleMouseConsole(WorldState *state) {
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

static void _handleKeyboard(WorldState *state){
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

static void _handleMouse(WorldState *state){
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Keyboard *k = appGetKeyboard(appGet());
   MouseEvent event = { 0 };
   Int2 pos = mouseGetPosition(mouse);
   Viewport *vp = state->view->viewport;
   Recti vpArea = { vp->region.origin_x, vp->region.origin_y, vp->region.origin_x + vp->region.width, vp->region.origin_y + vp->region.height };

   while (mousePopEvent(mouse, &event)){
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

void _boardHandleInput(WorldState *state, GameStateHandleInput *m){

   if (consoleGetEnabled(state->view->console)) {
      _handleKeyboardConsole(state);
      _handleMouseConsole(state);
   }
   else {
      _handleKeyboard(state);
      _handleMouse(state);
   }   
}

void _boardRender(WorldState *state, GameStateRender *m){
   renderManagerRender(state->view->managers->renderManager, m->frame);
}

static void _testWordWrap(WorldState *state) {
   RichText *rt = richTextCreateFromRaw("[c=0,1]The [i]lua_pushvalue function [/c]pushes on the [/i]top of the st[i]ack[/i] a copy of the element at the given index; lua_remove removes the element at the given index, shifting down all elements on top of that position to fill in the gap; lua_insert moves the top element into the given position, shifting up all elements on top of that position to open space; finally, lua_replace pops a value from the top and sets it as the value of the given index, without moving anything. Notice that the following operations have no effect on the stack:");
   //RichText *rt = richTextCreateFromRaw("the      emergencybroadcast system");
   vec(RichTextLine) *output = vecCreate(RichTextLine)(&richTextLineDestroy);
   FILE *f = fopen("testout.txt", "wb");
   String *str = stringCreate("");

   richTextGet(rt, str);
   fprintf(f, c_str(str));
   fprintf(f, "\n\n");
   
   stringClear(str);
   richTextGetRaw(rt, str);
   fprintf(f, c_str(str));
   fprintf(f, "\n\n-------------------------\n\n\n");
   
   richTextRenderToLines(rt, 5, output);

   
   fprintf(f, "->| ");
   vecForEach(RichTextLine, line, output, {
      vecForEach(Span, span, *line,{
         fprintf(f, c_str(span->string));
      });
   fprintf(f, " |<-\n\n->| ");
   });

   fprintf(f, "\n\n-----------WITH TAGS--------------\n\n\n");

   fprintf(f, "->| ");
   vecForEach(RichTextLine, line, output, {
      vecForEach(Span, span, *line,{
         stringClear(str);
         spanRenderToString(span, str);
         fprintf(f, c_str(str));
      });
      fprintf(f, " |<-\n\n->| ");
   });
   
   fclose(f);

   stringDestroy(str);
   vecDestroy(RichTextLine)(output);
   richTextDestroy(rt);
}

static void _addActor(WorldState *state, int x, int y, int imgX, int imgY) {
   Tile *t = gridManagerTileAtXY(state->view->managers->gridManager, x, y);
   Entity *e = entityCreate(state->view->entitySystem);
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, SizeComponent, 14, 14);
   COMPONENT_ADD(e, RectangleComponent, 0);

   COMPONENT_ADD(e, ImageComponent, .filename = stringIntern("assets/img/tiles.ega"), .partial = true, .x = imgX, .y = imgY, .width = 14, .height = 14);
   COMPONENT_ADD(e, LayerComponent, LayerGrid);
   COMPONENT_ADD(e, InViewComponent, 0);
   COMPONENT_ADD(e, GridComponent, x, y);
   COMPONENT_ADD(e, LightComponent, .radius = 0, .centerLevel = 0, .fadeWidth = 0);
   COMPONENT_ADD(e, ActorComponent, .moveTime = DEFAULT_MOVE_SPEED, .moveDelay = DEFAULT_MOVE_DELAY);

   if (t) {
      t->schema = 3;
      t->collision = 0;
   }

   entityUpdate(e);
}

static void _addTestEntities(WorldState *state) {
   int i;
   Entity *e = entityCreate(state->view->entitySystem);
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/bg.ega"));
   COMPONENT_ADD(e, LayerComponent, LayerBackground);
   COMPONENT_ADD(e, RenderedUIComponent, 0);
   entityUpdate(e);

   {
      StringView boxName = stringIntern("smallbox");
      textBoxManagerCreateTextBox(state->view->managers->textBoxManager, boxName, (Recti) { 15, 22, 38, 24 });
      textBoxManagerPushText(state->view->managers->textBoxManager, boxName, "You are likely to be eaten by a [c=0,13]grue[/c].");
      textBoxManagerShowTextArea(state->view->managers->textBoxManager, boxName);
      
   }

   for (i = 0; i < 15; ++i) {
      int x = appRand(appGet(), 0, 21);
      int y = appRand(appGet(), 0, 11);
      int sprite = appRand(appGet(), 0, 3);

      _addActor(state, x, y, 70 + (sprite*14), 28);
   }

}

static void _enterState(WorldState *state) {
   appLoadPalette(appGet(), "assets/pal/default.pal");
   cursorManagerCreateCursor(state->view->managers->cursorManager);
   pcManagerCreatePC(state->view->managers->pcManager);
   verbManagerCreateVerbs(state->view->managers->verbManager);
   

   gridManagerSetAmbientLight(state->view->managers->gridManager, STARTING_AMBIENT_LEVEL);

   _addTestEntities(state);

   //_testWordWrap(state);
}

StateClosure gameStateCreateWorld(WorldView *view){
   StateClosure out;
   WorldState *state = checkedCalloc(1, sizeof(WorldState));
   state->view = view;

   _enterState(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_board, &_boardStateDestroy);

   return out;
}