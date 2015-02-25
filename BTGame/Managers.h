#pragma once

typedef struct RenderManager_t RenderManager;

typedef struct {
   RenderManager *renderManager;
}BTManagers;

RenderManager *createRenderManager();