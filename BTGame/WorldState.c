#include "WorldView.h"
#include "Managers.h"
#include "LightGrid.h"
#include "Console.h"
#include "GameState.h"
#include "GameHelpers.h"
#include "ImageLibrary.h"
#include "ChoicePrompt.h"
#include "Weather.h"
#include "TextArea.h"
#include "Calendar.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"

#include "segashared\CheckedMemory.h"


#define STARTING_AMBIENT_LEVEL MAX_BRIGHTNESS

typedef struct {
   WorldView *view;

   TextArea *smallbox;

   ManagedImage *bg;
}WorldState;

static void _worldStateCreate(WorldState *self) { 
   testRain(self->view->weather);

}
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
   WorldView *view = state->view;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(view->cursorManager, mousePos.x, mousePos.y);
   actorManagerUpdate(view->actorManager);
   pcManagerUpdate(view->pcManager);
   textAreaUpdate(state->smallbox);

   calendarUpdate(view->calendar);
   calendarSetAmbientByTime(view->calendar);
   calendarSetPaletteByTime(view->calendar);
}




void _worldEnter(WorldState *state, StateEnter *m) {
   WorldView *view = state->view;
   state->smallbox = textAreaManagerGet(state->view->textAreaManager, stringIntern("smallbox"));
   state->bg = imageLibraryGetImage(state->view->imageLibrary, stringIntern(IMG_BG));


   verbManagerSetEnabled(view->verbManager, true);

}
void _worldExit(WorldState *state, StateExit *m) {
   WorldView *view = state->view;

   managedImageDestroy(state->bg);
   verbManagerSetEnabled(view->verbManager, false);
}

static void _handleKeyboard(WorldState *state) {
   WorldView *view = state->view;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   Viewport *vp = state->view->viewport;
   int speed = 2;

   while (keyboardPopEvent(k, &e)) {

      //intercept keyboard input for choicePrompt
      if (choicePromptHandleKeyEvent(view->choicePrompt, &e)) {
         continue;
      }

      if (e.action == SegaKey_Pressed) {
         switch (e.key) {
         case SegaKey_LeftControl:
            pcManagerSetSneak(view->pcManager, true);
            break;
         case SegaKey_1:
            verbManagerKeyButton(view->verbManager, Verb_Look, e.action);
            break;
         case SegaKey_2:
            verbManagerKeyButton(view->verbManager, Verb_Use, e.action);
            break;
         case SegaKey_3:
            verbManagerKeyButton(view->verbManager, Verb_Talk, e.action);
            break;
         case SegaKey_4:
            verbManagerKeyButton(view->verbManager, Verb_Fight, e.action);
            break;
         }
      }
      else if (e.action == SegaKey_Released) {
         switch (e.key) {
         case SegaKey_GraveAccent:
            fsmPush(state->view->gameState, gameStateCreateConsole(state->view));
            break;
         case SegaKey_LeftControl:
            pcManagerSetSneak(view->pcManager, false);
            break;
         case SegaKey_Escape:
            appQuit(appGet());
            break;
         case SegaKey_T:
            pcManagerToggleTorch(view->pcManager);
            break;
         case SegaKey_W:
         case SegaKey_A:
         case SegaKey_S:
         case SegaKey_D:
            pcManagerStop(view->pcManager);
            break;
         case SegaKey_1:
            verbManagerKeyButton(view->verbManager, Verb_Look, e.action);
            break;
         case SegaKey_2:
            verbManagerKeyButton(view->verbManager, Verb_Use, e.action);
            break;
         case SegaKey_3:
            verbManagerKeyButton(view->verbManager, Verb_Talk, e.action);
            break;
         case SegaKey_4:
            verbManagerKeyButton(view->verbManager, Verb_Fight, e.action);
            break;
         }
      }
   }

   if (keyboardIsDown(k, SegaKey_W)) {
      pcManagerMoveRelative(view->pcManager, 0, -1);
   }
   if (keyboardIsDown(k, SegaKey_S)) {
      pcManagerMoveRelative(view->pcManager, 0, 1);
   }
   if (keyboardIsDown(k, SegaKey_A)) {
      pcManagerMoveRelative(view->pcManager, -1, 0);
   }
   if (keyboardIsDown(k, SegaKey_D)) {
      pcManagerMoveRelative(view->pcManager, 1, 0);
   }
}

static void _handleMouse(WorldState *state) {
   WorldView *view = state->view;
   Mouse *mouse = appGetMouse(appGet());
   Keyboard *k = appGetKeyboard(appGet());
   MouseEvent event = { 0 };
   Int2 pos = mouseGetPosition(mouse);
   Viewport *vp = state->view->viewport;
   Recti vpArea = { vp->region.origin_x, vp->region.origin_y, vp->region.origin_x + vp->region.width, vp->region.origin_y + vp->region.height };

   while (mousePopEvent(mouse, &event)) {

      if (choicePromptHandleMouseEvent(state->view->choicePrompt, &event)) {
         continue;
      }

      if (event.action == SegaMouse_Scrolled) {
      }
      else if (event.action == SegaMouse_Pressed) {
         switch (event.button) {
         case SegaMouseBtn_Left:
            if (verbManagerMouseButton(view->verbManager, &event)) {
               continue;
            }
            break;
         }
      }
      else if (event.action == SegaMouse_Released) {
         switch (event.button) {
         case SegaMouseBtn_Left:

            if (verbManagerMouseButton(view->verbManager, &event)) {
               continue;
            }
            break;
         case SegaMouseBtn_Right:

            break;
         }
      }

   }

   if (mouseIsDown(mouse, SegaMouseBtn_Right)) {
      if (rectiContains(vpArea, pos)) {
         pcManagerMove(view->pcManager,
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
   Frame *frame = m->frame;
   frameClear(frame, FrameRegionFULL, 0);
   
   frameRenderImage(m->frame, FrameRegionFULL, 0, 0, managedImageGetImage(state->bg));

   gridManagerRender(state->view->gridManager, frame);

   actorManagerRender(state->view->actorManager, frame);

   weatherRender(state->view->weather, frame);
   gridManagerRenderLighting(state->view->gridManager, frame);

   verbManagerRender(state->view->verbManager, frame);

   calendarRenderClock(state->view->calendar, m->frame);

   choicePromptRender(state->view->choicePrompt, frame);
   textAreaRender(state->smallbox, state->view, frame);
   cursorManagerRender(state->view->cursorManager, frame);

   framerateViewerRender(state->view->framerateViewer, frame);
   consoleRenderNotification(state->view->console, frame);
}

StateClosure gameStateCreateWorld(WorldView *view){
   StateClosure out;
   WorldState *state = checkedCalloc(1, sizeof(WorldState));
   state->view = view;

   _worldStateCreate(state);
   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_world, &_worldStateDestroy);

   return out;
}