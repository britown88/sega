#pragma once

typedef struct RenderManager_t RenderManager;
typedef struct EntitySystem_t EntitySystem;
typedef struct Frame_t Frame;

typedef struct {
   RenderManager *renderManager;
}BTManagers;

RenderManager *createRenderManager(EntitySystem *system);
void renderManagerRender(RenderManager *self, Frame *frame);