#pragma once

typedef struct RenderManager_t RenderManager;
typedef struct EntitySystem_t EntitySystem;
typedef struct ImageManager_t ImageManager;
typedef struct Frame_t Frame;

typedef struct {
   RenderManager *renderManager;
}BTManagers;

RenderManager *createRenderManager(EntitySystem *system, ImageManager *imageManager);
void renderManagerRender(RenderManager *self, Frame *frame);