#include "WorldView.h"
#include "Managers.h"
#include "LightGrid.h"
#include "Console.h"
#include "GameState.h"
#include "GameHelpers.h"
#include "ImageLibrary.h"
#include "MapEditor.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "GameClock.h"
#include "Weather.h"

#include "segashared\CheckedMemory.h"

#define VP_SPEED 3
#define VP_FAST_SPEED 8

typedef struct {
   WorldView *view;
   MapEditor *editor;
   ManagedImage *bg;
   bool pop;
}EditorState;

static void _editorStateCreate(EditorState *state) {
   state->editor = mapEditorCreate(state->view);
}
static void _editorStateDestroy(EditorState *self) {
   mapEditorDestroy(self->editor);
   checkedFree(self);
}

static void _editorUpdate(EditorState*, GameStateUpdate*);
static void _editorHandleInput(EditorState*, GameStateHandleInput*);
static void _editorRender(EditorState*, GameStateRender*);
static void _editorEnter(EditorState*, StateEnter*);
static void _editorExit(EditorState*, StateExit*);

static void _editor(EditorState *state, Type *t, Message m) {
   if (t == GetRTTI(GameStateUpdate)) { _editorUpdate(state, m); }
   else if (t == GetRTTI(GameStateHandleInput)) { _editorHandleInput(state, m); }
   else if (t == GetRTTI(GameStateRender)) { _editorRender(state, m); }
   else if (t == GetRTTI(StateEnter)) { _editorEnter(state, m); }
   else if (t == GetRTTI(StateExit)) { _editorExit(state, m); }
}


void _editorEnter(EditorState *state, StateEnter *m) {
   state->bg = imageLibraryGetImage(state->view->imageLibrary, stringIntern(IMG_BG_EDITOR));
   mapEditorReset(state->editor);
}
void _editorExit(EditorState *state, StateExit *m) {
   managedImageDestroy(state->bg);
}

void _editorUpdate(EditorState *state, GameStateUpdate *m) {
   WorldView *view = state->view;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(view->cursorManager, mousePos.x, mousePos.y);
   
   if (state->pop) {
      fsmPop(state->view->gameState);
   }

}

static void _handleKeyboard(EditorState *state) {
   WorldView *view = state->view;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   Viewport *vp = view->viewport;
   short speed;

   while (keyboardPopEvent(k, &e)) {
      if (e.action == SegaKey_Released) {
         switch (e.key) {
         case SegaKey_Escape:
            state->pop = true;
            break;
         case SegaKey_GraveAccent:
            fsmPush(view->gameState, gameStateCreateConsole(view));
            break;
         }
      }
   }

   speed = keyboardIsDown(k, SegaKey_LeftShift) ? VP_FAST_SPEED : VP_SPEED;

   if (keyboardIsDown(k, SegaKey_W)) {
      vp->worldPos.y = MAX(0, vp->worldPos.y - speed);
   }
   if (keyboardIsDown(k, SegaKey_S)) {
      short maxY = (gridManagerHeight(view->gridManager) * GRID_CELL_SIZE) - vp->region.height;
      vp->worldPos.y = MIN(maxY, vp->worldPos.y + speed);
   }
   if (keyboardIsDown(k, SegaKey_A)) {
      vp->worldPos.x = MAX(0, vp->worldPos.x - speed);
   }
   if (keyboardIsDown(k, SegaKey_D)) {
      short maxX = (gridManagerWidth(view->gridManager) * GRID_CELL_SIZE) - vp->region.width;
      vp->worldPos.x = MIN(maxX, vp->worldPos.x + speed);
   }
}

static void _handleMouse(EditorState *state) {
   Mouse *mouse = appGetMouse(appGet());
   MouseEvent event = { 0 };
   Int2 pos = mouseGetPosition(mouse);
   Viewport *vp = state->view->viewport;
   MapEditor *me = state->editor;
   Recti vpArea = { 
      vp->region.origin_x, 
      vp->region.origin_y, 
      vp->region.origin_x + vp->region.width, 
      vp->region.origin_y + vp->region.height 
   };
   bool gridOperation = rectiContains(vpArea, pos);
   bool schemaOperation = mapEditorPointInSchemaWindow(me, pos);

   while (mousePopEvent(mouse, &event)) {            

      if (event.action == SegaMouse_Scrolled) {
         if (schemaOperation) {
            mapEditorScrollSchemas(me, event.pos.y);
         }
      }
      else if (event.action == SegaMouse_Pressed) {
         if (schemaOperation) {
            mapEditorSelectSchema(me, pos);
         }
         else if (gridOperation && event.button == SegaMouseBtn_Right) {
            Tile *t = gridManagerTileAtScreenPos(state->view->gridManager, pos.x, pos.y);

            if (t) {
               mapEditorSetSelectedSchema(me, tileGetSchema(t));
            }
            
         }
      }
   }

   if (gridOperation) {
      Int2 vpPos = screenToWorld(state->view, pos);
      vpPos.x /= GRID_CELL_SIZE;
      vpPos.y /= GRID_CELL_SIZE;

      mapEditorUpdateStats(me, vpPos);

      if (mouseIsDown(mouse, SegaMouseBtn_Left)) {
         size_t t = gridManagerCellIDFromXY(state->view->gridManager, vpPos.x, vpPos.y);
         if (t < INF) {
            gridManagerChangeTileSchema(state->view->gridManager, t, mapEditorGetSelectedSchema(me));
         }
      }
   }
}

void _editorHandleInput(EditorState *state, GameStateHandleInput *m) {
   _handleKeyboard(state);
   _handleMouse(state);
}

void _editorRender(EditorState *state, GameStateRender *m) {
   Frame *frame = m->frame;
   frameClear(frame, FrameRegionFULL, 0);

   gridManagerRender(state->view->gridManager, frame);
   actorManagerRender(state->view->actorManager, frame);
   weatherRender(state->view->weather, frame);
   gridManagerRenderLighting(state->view->gridManager, frame);   

   frameRenderImage(m->frame, FrameRegionFULL, 0, 0, managedImageGetImage(state->bg));
   mapEditorRenderSchemas(state->editor, m->frame);
   mapEditorRenderXYDisplay(state->editor, m->frame);
   cursorManagerRender(state->view->cursorManager, frame);
   framerateViewerRender(state->view->framerateViewer, frame);
}

StateClosure gameStateCreateEditor(WorldView *view) {
   StateClosure out;
   EditorState *state = checkedCalloc(1, sizeof(EditorState));
   state->view = view;

   _editorStateCreate(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_editor, &_editorStateDestroy);
   return out;
}
