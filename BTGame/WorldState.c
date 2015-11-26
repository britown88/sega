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
   textBoxManagerUpdate(managers->textBoxManager);
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
      else if (e.action == SegaKey_Pressed) {
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

static void _handleKeyboard(WorldState *state){
   BTManagers *managers = state->view->managers;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   Viewport *vp = state->view->viewport;
   int speed = 2;
   static int toggle = 1;
   static int amb = 0;

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
      t->collision = GRID_SOLID;
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
            cursorManagerClearVerb(state->view->managers->cursorManager);
            break;
         case SegaMouseBtn_Right:
            cursorManagerClearVerb(state->view->managers->cursorManager);
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
   }
   else {
      _handleKeyboard(state);
      _handleMouse(state);
   }   
}

void _boardRender(WorldState *state, GameStateRender *m){
   renderManagerRender(state->view->managers->renderManager, m->frame);
}

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static void _testLUA(WorldState *state) {
   RichText *rt = richTextCreateFromRaw("[c=1,3]This is a [i]12345656765765789[/i]test of the      emer\\ngency[i]broad\ncast [/c]system[/i]  which should work [    c  =  12      15     ] yessir\n");
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
   
   richTextRenderToLines(rt, 6, output);

   
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

static void _addTestEntities(WorldState *state) {
   Entity *e = entityCreate(state->view->entitySystem);
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/grid.ega"));
   COMPONENT_ADD(e, LayerComponent, LayerBackground);
   COMPONENT_ADD(e, RenderedUIComponent, 0);
   entityUpdate(e);

   {
      StringView boxName = stringIntern("smallbox");
      textBoxManagerCreateTextBox(state->view->managers->textBoxManager, boxName, (Recti) { 15, 22, 38, 24 });
      textBoxManagerPushText(state->view->managers->textBoxManager, boxName, "\\c0EThis \\c0Fwas \\c0Enot \\c0Fthe \\c0Emost \\c0Fefficient \\c0Eway \\c0Fto \\c0Euse \\c0Fthe \\c0Edisk \\c0Fsurface \\c0Ewith \\c0Favailable \\c0Edrive \\c0Felectronics;\\c0E[citation needed] \\c0Fbecause the sectors have constant angular size, the 512 bytes in each sector are compressed more near the disk's center. A more space-efficient technique would be to increase the number of sectors per track toward the outer edge of the disk, from 18 to 30 for instance, thereby keeping constant the amount of physical disk space used for storing each sector; an example is zone bit recording. Apple implemented this in early Macintosh computers by spinning the disk slower when the head was at the edge, while maintaining the data rate, allowing 400 KB of storage per side and an extra 160 KB on a double-sided disk.[38] This higher capacity came with a disadvantage: the format used a unique drive mechanism and control circuitry, meaning that Mac disks could not be read on other computers. Apple eventually reverted to constant angular velocity on HD floppy disks with their later machines, still unique to Apple as they supported the older variable-speed formats.");
      //textBoxManagerPushText(state->view->managers->textBoxManager, boxName, "supported the older variable-speed formats.");
      //textBoxManagerPushText(state->view->managers->textBoxManager, boxName, "You are likely to be eaten by a grue.");
   }

}

static void _enterState(WorldState *state) {
   appLoadPalette(appGet(), "assets/img/default2.pal");
   cursorManagerCreateCursor(state->view->managers->cursorManager);
   pcManagerCreatePC(state->view->managers->pcManager);
   verbManagerCreateVerbs(state->view->managers->verbManager);
   consoleCreateLines(state->view->console);

   gridManagerSetAmbientLight(state->view->managers->gridManager, 2);   

   _addTestEntities(state);

   //_testLUA(state);
}

StateClosure gameStateCreateWorld(WorldView *view){
   StateClosure out;
   WorldState *state = checkedCalloc(1, sizeof(WorldState));
   state->view = view;

   _enterState(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_board, &_boardStateDestroy);

   return out;
}