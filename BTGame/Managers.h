#pragma once

typedef struct RenderManager_t RenderManager;
typedef struct CursorManager_t CursorManager;
typedef struct EntitySystem_t EntitySystem;
typedef struct ImageManager_t ImageManager;
typedef struct GridManager_t GridManager;
typedef struct Frame_t Frame;

typedef struct {
   RenderManager *renderManager;
   CursorManager *cursorManager;
   GridManager *gridManager;

}BTManagers;

RenderManager *createRenderManager(EntitySystem *system, ImageManager *imageManager, double *fps);
void renderManagerRender(RenderManager *self, Frame *frame);

CursorManager *createCursorManager(EntitySystem *system);
void cursorManagerCreateCursor(CursorManager *self);
void cursorManagerUpdate(CursorManager *self, int x, int y);

GridManager *createGridManager(EntitySystem *system);
void gridManagerUpdate(GridManager *self);