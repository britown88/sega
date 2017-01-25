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
#define SCALE_INC 0.1f
#define SCALE_TIME_US 500000

typedef enum {
   None = 0,
   Paint,
   Square
}PaintStates;

typedef struct {
   Viewport gridRenderingVP;
   vec(Vertex) *vbo;
   vec(size_t) *ibo;
   FrameRegion gridRegion;
   Texture *fbo;
}MeshRenderData;

MeshRenderData *meshRenderDataCreate() {
   MeshRenderData *out = checkedCalloc(1, sizeof(MeshRenderData));
   out->vbo = vecCreate(Vertex)(NULL);
   out->ibo = vecCreate(size_t)(NULL);

   vecPushStackArray(Vertex, out->vbo, {
      { .coords = { 0.0f, 0.0f, 0.0f },.texCoords = { 0, 0 } },
      { .coords = { 1.0f, 0.0f, 0.0f },.texCoords = { 1.0, 0 } },
      { .coords = { 0.0f, 1.0f, 0.0f },.texCoords = { 0, 1.0 } },
      { .coords = { 1.0f, 1.0f, 0.0f },.texCoords = { 1.0, 1.0 } },
   });

   vecPushStackArray(size_t, out->ibo, { 0, 2, 1, 2, 3, 1 });

   out->gridRegion = (FrameRegion) {
      .origin_x = GRID_POS_X,
         .origin_y = GRID_POS_Y,
         .width = GRID_SIZE_X,
         .height = GRID_SIZE_Y
   };

   out->fbo = textureCreate(out->gridRegion.width, out->gridRegion.height);

   return out;
}

void meshRenderDataDestroy(MeshRenderData *self) {
   textureDestroy(self->fbo);
   vecDestroy(Vertex)(self->vbo);
   vecDestroy(size_t)(self->ibo);
   checkedFree(self);
}

typedef struct {
   WorldView *view;
   MapEditor *editor;
   ManagedImage *bg;
   bool pop;

   int lightDebugSelection;
   Int2 p1, p2;

   PaintStates state;
   Int2 squareStart, squareEnd;

   MeshRenderData *gridMesh;

   float targetStartScale, scale, targetScale;
   float minScale, maxScale;
   Microseconds scaleStartTime;

}EditorState;

static void _setScaleRanges(EditorState *state) {
   state->minScale = 0.1f;
   state->maxScale = 1000000.0f;
}

static void _increaseScale(EditorState *state) {
   state->targetStartScale = state->scale;
   state->targetScale = MIN(state->maxScale, state->targetScale + SCALE_INC * state->scale );
   state->scaleStartTime = appGetTime(appGet());
}
static void _decreaseScale(EditorState *state) {
   state->targetStartScale = state->scale;
   state->targetScale = MAX(state->minScale, state->targetScale - SCALE_INC * state->scale);
   state->scaleStartTime = appGetTime(appGet());
}
static void _resetScale(EditorState *state) {
   state->targetStartScale = state->scale;
   state->targetScale = 1.0f;
   state->scaleStartTime = appGetTime(appGet());
}
static bool _scaleIsReset(EditorState *state) {
   return fabs(state->scale - 1.0f) < 0.001f && fabs(state->targetScale - 1.0f) < 0.001f;
}

static void _updateScale(EditorState *state) {   
   Microseconds deltaTime = appGetTime(appGet()) - state->scaleStartTime;

   if (deltaTime < SCALE_TIME_US) {
      float deltaScale = state->targetScale - state->targetStartScale;
      state->scale = state->targetStartScale + (deltaTime / (float)SCALE_TIME_US) * deltaScale;
   }
   else {
      state->scale = state->targetScale;
   }
}

static void _editorStateCreate(EditorState *state) {
   state->editor = mapEditorCreate(state->view);
   state->targetStartScale = state->scale = state->targetScale = 1.0f;
   state->gridMesh = meshRenderDataCreate();
}
static void _editorStateDestroy(EditorState *self) {
   mapEditorDestroy(self->editor);
   meshRenderDataDestroy(self->gridMesh);
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

   _setScaleRanges(state);
}
void _editorExit(EditorState *state, StateExit *m) {
   managedImageDestroy(state->bg);
   calendarResume(state->view->calendar);
}

void _editorUpdate(EditorState *state, GameStateUpdate *m) {
   WorldView *view = state->view;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   _updateScale(state);
   
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

      gridManagerSetTileCollision(gm, t, 0);
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
               gridManagerSetTileCollision(state->view->gridManager, tile, 0);
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
         gridManagerSetTileCollision(state->view->gridManager, tile, 0);
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
      int calendarOperation = calendarEditorMouse(state->view->calendar, &event, pos);

      if (event.action == SegaMouse_Scrolled) {
         
         if (schemaOperation) {
            mapEditorScrollSchemas(me, event.pos.y);
         }
         else if(!calendarOperation){
            if (event.pos.y > 0) {
               _decreaseScale(state);
            }
            else {
               _increaseScale(state);
            }
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

void renderGridMesh(EditorState *state, Texture *frame) {
   MeshRenderData *d = state->gridMesh;

   int fboWidth = GRID_SIZE_X * state->scale;
   int fboHeight = GRID_SIZE_Y * state->scale;
   Transform modelTrans = { .size = (Int3) { d->gridRegion.width, d->gridRegion.height } };
   Transform texTrans = { .size = (Int3) { fboWidth, fboHeight } };

   Viewport *oldview = state->view->viewport;
   d->gridRenderingVP = (Viewport) {
      .region = { 0, 0, fboWidth, fboHeight },
      .worldPos = oldview->worldPos
   };

   textureResize(d->fbo, fboWidth, fboHeight);
   textureClear(d->fbo, NULL, 0);

   state->view->viewport = &d->gridRenderingVP;

   gridManagerRender(state->view->gridManager, d->fbo);
   actorManagerRender(state->view->actorManager, d->fbo);
   weatherRender(state->view->weather, d->fbo);
   gridManagerRenderLighting(state->view->gridManager, d->fbo);

   state->view->viewport = oldview;

   textureRenderMesh(frame, &d->gridRegion, d->vbo, d->ibo, d->fbo, modelTrans, texTrans);
}


void _editorRender(EditorState *state, GameStateRender *m) {
   Texture *frame = m->frame;
   textureClear(frame, NULL, 0);

   if (fabs(state->scale - 1.0f) < 0.0001f){
      gridManagerRender(state->view->gridManager, frame);
      actorManagerRender(state->view->actorManager, frame);
      weatherRender(state->view->weather, frame);
      gridManagerRenderLighting(state->view->gridManager, frame);
   }
   else {
      renderGridMesh(state, frame);
   }

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

   //
}

StateClosure gameStateCreateEditor(WorldView *view) {
   StateClosure out;
   EditorState *state = checkedCalloc(1, sizeof(EditorState));
   state->view = view;

   _editorStateCreate(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_editor, &_editorStateDestroy);
   return out;
}
