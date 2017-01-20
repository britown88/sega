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
#include "LightDebugger.h"
#include "Calendar.h"

#include "MeshRendering.h"
#include "segautils/Math.h"
#include <math.h>

#include "segashared\CheckedMemory.h"

#include "segautils/StandardVectors.h"

#define VP_SPEED 3
#define VP_FAST_SPEED 8

typedef enum {
   None = 0,
   Paint,
   Square
}PaintStates;

typedef struct {
   WorldView *view;
   MapEditor *editor;
   ManagedImage *bg;
   bool pop;

   int lightDebugSelection;
   Int2 p1, p2;

   PaintStates state;
   Int2 squareStart, squareEnd;
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
   calendarPause(state->view->calendar);
   pcManagerUpdate(state->view->pcManager);
}
void _editorExit(EditorState *state, StateExit *m) {
   managedImageDestroy(state->bg);
   calendarResume(state->view->calendar);
}

void _editorUpdate(EditorState *state, GameStateUpdate *m) {
   WorldView *view = state->view;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   
   cursorManagerUpdate(view->cursorManager, mousePos.x, mousePos.y);
   calendarUpdate(view->calendar);
   calendarSetAmbientByTime(view->calendar);
   calendarSetPaletteByTime(view->calendar);
   
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

   bool vpChange = false;

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

      else if (e.action == SegaKey_Pressed || e.action == SegaKey_Repeat) {
         if (e.key == SegaKey_Z && keyboardIsDown(k, SegaKey_LeftControl)) {
            gridManagerUndo(state->view->gridManager);
         }
         else if (e.key == SegaKey_Y && keyboardIsDown(k, SegaKey_LeftControl)) {
            gridManagerRedo(state->view->gridManager);
         }
      }
   }

   speed = keyboardIsDown(k, SegaKey_LeftShift) ? VP_FAST_SPEED : VP_SPEED;

   if (keyboardIsDown(k, SegaKey_W)) {
      vp->worldPos.y = MAX(0, vp->worldPos.y - speed);
      vpChange = true;
   }
   if (keyboardIsDown(k, SegaKey_S)) {
      short maxY = (gridManagerHeight(view->gridManager) * GRID_CELL_SIZE) - vp->region.height;
      vp->worldPos.y = MIN(maxY, vp->worldPos.y + speed);
      vpChange = true;
   }
   if (keyboardIsDown(k, SegaKey_A)) {
      vp->worldPos.x = MAX(0, vp->worldPos.x - speed);
      vpChange = true;
   }
   if (keyboardIsDown(k, SegaKey_D)) {
      short maxX = (gridManagerWidth(view->gridManager) * GRID_CELL_SIZE) - vp->region.width;
      vp->worldPos.x = MIN(maxX, vp->worldPos.x + speed);
      vpChange = true;
   }

   if (vpChange && state->state == Square) {
      Int2 pos = mouseGetPosition(appGetMouse(appGet()));
      Int2 vpPos = screenToWorld(state->view, pos);
      vpPos.x /= GRID_CELL_SIZE;
      vpPos.y /= GRID_CELL_SIZE;

      state->squareEnd = vpPos;
   }
}

static void lightDebugClick(EditorState *state, Int2 pos) {   
   Int2 vpPos = screenToWorld(state->view, pos);
   vpPos.x /= GRID_CELL_SIZE;
   vpPos.y /= GRID_CELL_SIZE;

   if (state->lightDebugSelection == 0) {
      state->p1 = vpPos;
      ++state->lightDebugSelection;
   }
   else if (state->lightDebugSelection == 1) {
      state->p2 = vpPos;
      gridManagerDebugLights(state->view->gridManager, state->p1, state->p2);
      ++state->lightDebugSelection;
   }
   else {
      state->lightDebugSelection = 0;
      lightDebuggerClear(state->view->lightDebugger);
   }
}

static void _addTileToFloodFill(GridManager *gm, byte baseSchema, int x, int y, vec(size_t) *openList, vec(size_t) *closedList) {
   size_t t = gridManagerCellIDFromXY(gm, x, y);

   if (x >= 0 && x < gridManagerWidth(gm) && y >= 0 && y < gridManagerHeight(gm)) {
      Tile *tile = gridManagerTileAt(gm, t);

      if (tile && tileGetSchema(tile) == baseSchema) {
         if (vecIndexOf(size_t)(closedList, &t) == INF) {
            if (gridManagerTileAt(gm, t)) {
               vecPushBack(size_t)(openList, &t);
               return;
            }
         }
      }
   }
   
   vecPushBack(size_t)(closedList, &t);   
}

static void _floodFill(EditorState *state, byte baseSchema, byte selectedSchema, vec(size_t) *openList, vec(size_t) *closedList) {
   GridManager *gm = state->view->gridManager;
   
   size_t t = *vecAt(size_t)(openList, 0);
   Tile *tile = gridManagerTileAt(gm, t);

   if (tile && tileGetSchema(tile) == baseSchema) {
      int x, y;

      gridManagerChangeTileSchema(gm, t, selectedSchema);
      gridManagerXYFromCellID(gm, t, &x, &y);

      _addTileToFloodFill(gm, baseSchema, x - 1, y, openList, closedList);
      _addTileToFloodFill(gm, baseSchema, x , y - 1, openList, closedList);
      _addTileToFloodFill(gm, baseSchema, x + 1, y, openList, closedList);
      _addTileToFloodFill(gm, baseSchema, x, y + 1, openList, closedList);
   }

   vecRemoveAt(size_t)(openList, 0);
   vecPushBack(size_t)(closedList, &t);
}

static void gridClickDown(EditorState *state, Int2 pos) {
   GridManager *gm = state->view->gridManager;
   Keyboard *k = appGetKeyboard(appGet());
   Int2 vpPos = screenToWorld(state->view, pos);
   vpPos.x /= GRID_CELL_SIZE;
   vpPos.y /= GRID_CELL_SIZE;

   if (keyboardIsDown(k, SegaKey_LeftShift)) {
      //start a quare
      state->state = Square;
      state->squareStart = state->squareEnd = vpPos;
   }
   else if (keyboardIsDown(k, SegaKey_LeftControl)) {
      //do a fill
      size_t tileID = gridManagerCellIDFromXY(gm, vpPos.x, vpPos.y);
      if (tileID < INF) {
         Tile *t = gridManagerTileAt(gm, tileID);
         byte schema = tileGetSchema(t);
         byte selectedSchema = mapEditorGetSelectedSchema(state->editor);

         if (schema != selectedSchema) {
            vec(size_t) *openList = vecCreate(size_t)(NULL);
            vec(size_t) *closedList = vecCreate(size_t)(NULL);

            vecPushBack(size_t)(openList, &tileID);
            while (!vecIsEmpty(size_t)(openList)) {
               _floodFill(state, schema, selectedSchema, openList, closedList);
            }

            vecDestroy(size_t)(openList);
            vecDestroy(size_t)(closedList);
         }
      }

      state->state = None;
      gridManagerSaveSnapshot(gm);
   }
   else {
      //regular painting
      state->state = Paint;
   }
}

static void gridClickUp(EditorState *state, Int2 pos) {
   if (state->state == Square) {
      MapEditor *me = state->editor;
      Int2 vpPos = screenToWorld(state->view, pos);
      vpPos.x /= GRID_CELL_SIZE;
      vpPos.y /= GRID_CELL_SIZE;
      state->squareEnd = vpPos;

      int x, y;      
      int startX = state->squareStart.x < state->squareEnd.x ? state->squareStart.x : state->squareEnd.x;
      int startY = state->squareStart.y < state->squareEnd.y ? state->squareStart.y : state->squareEnd.y;
      int endX = state->squareStart.x < state->squareEnd.x ? state->squareEnd.x : state->squareStart.x;
      int endY = state->squareStart.y < state->squareEnd.y ? state->squareEnd.y : state->squareStart.y;

      for (y = startY; y <= endY; ++y) {
         for (x = startX; x <= endX; ++x) {
            size_t tile = gridManagerCellIDFromXY(state->view->gridManager, x, y);
            if (tile < INF) {
               gridManagerChangeTileSchema(state->view->gridManager, tile, mapEditorGetSelectedSchema(me));
            }
         }
      }
   }

   if (state->state != None) {
      gridManagerSaveSnapshot(state->view->gridManager);
   }

   state->state = None;

}

static void gridClickMove(EditorState *state, Int2 pos) {
   Mouse *mouse = appGetMouse(appGet());
   Keyboard *k = appGetKeyboard(appGet());
   MapEditor *me = state->editor;
   Int2 vpPos = screenToWorld(state->view, pos);
   size_t tile;
   vpPos.x /= GRID_CELL_SIZE;
   vpPos.y /= GRID_CELL_SIZE;

   mapEditorUpdateStats(me, vpPos);

   switch (state->state) {
   case Paint:
      tile = gridManagerCellIDFromXY(state->view->gridManager, vpPos.x, vpPos.y);
      if (tile < INF) {
         gridManagerChangeTileSchema(state->view->gridManager, tile, mapEditorGetSelectedSchema(me));
      }
      break;

   case Square:
      state->squareEnd = vpPos;
      break;
   }
}

static void _handleMouse(EditorState *state) {
   Mouse *mouse = appGetMouse(appGet());
   MouseEvent event = { 0 };
   Keyboard *k = appGetKeyboard(appGet());
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
      calendarEditorMouse(state->view->calendar, &event, pos);

      if (event.action == SegaMouse_Scrolled) {
         
         if (schemaOperation) {
            mapEditorScrollSchemas(me, event.pos.y);
         }
      }
      else if (event.action == SegaMouse_Pressed) {
         if (schemaOperation) {
            mapEditorSelectSchema(me, pos);
         }
         else if (gridOperation) {
            if (event.button == SegaMouseBtn_Right) {
               if (keyboardIsDown(k, SegaKey_LeftShift)) {
                  lightDebugClick(state, pos);
               }
               else {
                  Tile *t = gridManagerTileAtScreenPos(state->view->gridManager, pos.x, pos.y);
                  if (t) {
                     mapEditorSetSelectedSchema(me, tileGetSchema(t));
                  }
               }
            }
            
            else if(event.button == SegaMouseBtn_Left){
               gridClickDown(state, pos);
            }
         }
      }
      else if (event.action == SegaMouse_Released  && event.button == SegaMouseBtn_Left) {
         gridClickUp(state, pos);

      }
   }

   if (gridOperation) {
      gridClickMove(state, pos);
   }
}

void _editorHandleInput(EditorState *state, GameStateHandleInput *m) {
   _handleKeyboard(state);
   _handleMouse(state);
}

static void _renderSquare(EditorState *state, Texture *tex) {
   int x, y;
   GridManager *gm = state->view->gridManager;
   Viewport *vp = state->view->viewport;

   int startX = state->squareStart.x < state->squareEnd.x ? state->squareStart.x : state->squareEnd.x;
   int startY = state->squareStart.y < state->squareEnd.y ? state->squareStart.y : state->squareEnd.y;
   int endX = state->squareStart.x < state->squareEnd.x ? state->squareEnd.x : state->squareStart.x;
   int endY = state->squareStart.y < state->squareEnd.y ? state->squareEnd.y : state->squareStart.y;

   for (y = startY; y <= endY; ++y) {
      for (x = startX; x <= endX; ++x) {
         size_t tile = gridManagerCellIDFromXY(gm, x, y);
         if (tile < INF) {
            int renderX = x * GRID_CELL_SIZE - vp->worldPos.x;
            int renderY = y * GRID_CELL_SIZE - vp->worldPos.y;
            byte schema = mapEditorGetSelectedSchema(state->editor);
            
            textureRenderRect(tex, &vp->region, renderX, renderY, renderX + GRID_CELL_SIZE, renderY + GRID_CELL_SIZE, 0);
            gridManagerRenderSchema(gm, schema, tex, &vp->region, renderX, renderY);;
         }
      }
   }
}

void _renderTestMesh(EditorState *state, Texture *frame) {
   static int fboSize = 100;

   int fbowidth = fboSize += 1;
   int fboheight = fbowidth * ((float)GRID_SIZE_Y / GRID_SIZE_X) - 1;

   Viewport *oldv = state->view->viewport;
   Viewport tempv = { .region = {0, 0, fbowidth, fboheight }, .worldPos = oldv->worldPos };
   vec(Vertex) *vbo = vecCreate(Vertex)(NULL);
   vec(size_t) *ibo = vecCreate(size_t)(NULL);
   Texture *tex;
   Transform t = {
      .size = (Int3) { GRID_SIZE_X - 1, GRID_SIZE_Y - 1, 1 },
      .offset = (Int3) { GRID_POS_X, GRID_POS_Y, 0 },
      .rotation = quaternionFromAxisAngle((Float3){0}, 0.0f)
   };

   Texture *fbo = textureCreate(fbowidth, fboheight);
   textureClear(fbo, NULL, 0);

   vecPushStackArray(Vertex, vbo, {
      { .coords = { 0.0f, 0.0f, 0.0f },.texCoords = { 0, 0 } },
      { .coords = { 1.0f, 0.0f, 0.0f },.texCoords = { fbowidth, 0 } },
      { .coords = { 0.0f, 1.0f, 0.0f },.texCoords = { 0, fboheight } },
      { .coords = { 1.0f, 1.0f, 0.0f },.texCoords = { fbowidth, fboheight } },
   });

   vecPushStackArray(size_t, ibo, { 0, 2, 1, 2, 3, 1});

   state->view->viewport = &tempv;
   gridManagerRender(state->view->gridManager, fbo);
   actorManagerRender(state->view->actorManager, fbo);
   weatherRender(state->view->weather, fbo);
   gridManagerRenderLighting(state->view->gridManager, fbo);
   state->view->viewport = oldv;

   renderMesh(vbo, ibo, fbo, t, frame);

   vecDestroy(Vertex)(vbo);
   vecDestroy(size_t)(ibo);
   textureDestroy(fbo);

}

void _editorRender(EditorState *state, GameStateRender *m) {
   Texture *frame = m->frame;
   textureClear(frame, NULL, 0);

   //gridManagerRender(state->view->gridManager, frame);
   //actorManagerRender(state->view->actorManager, frame);
   //weatherRender(state->view->weather, frame);
   //gridManagerRenderLighting(state->view->gridManager, frame);  

   _renderTestMesh(state, frame);

   if (state->state == Square) {
      _renderSquare(state, frame);
   }

   textureRenderTexture(frame, NULL, 0, 0, managedImageGetTexture(state->bg));

   calendarRenderClock(state->view->calendar, m->frame);
  
   mapEditorRenderSchemas(state->editor, m->frame);
   mapEditorRenderXYDisplay(state->editor, m->frame);
   cursorManagerRender(state->view->cursorManager, frame);

   framerateViewerRender(state->view->framerateViewer, frame);
   lightDebuggerRender(state->view->lightDebugger, m->frame);

   
}

StateClosure gameStateCreateEditor(WorldView *view) {
   StateClosure out;
   EditorState *state = checkedCalloc(1, sizeof(EditorState));
   state->view = view;

   _editorStateCreate(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_editor, &_editorStateDestroy);
   return out;
}
