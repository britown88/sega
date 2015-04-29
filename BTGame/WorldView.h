#pragma once

typedef struct BTManagers_t BTManagers;
typedef struct EntitySystem_t EntitySystem;
typedef struct ImageLibrary_t ImageLibrary;
typedef struct FSM_t FSM;

typedef struct WorldView_t {
   BTManagers *managers;
   EntitySystem *entitySystem;
   ImageLibrary *imageLibrary;
   FSM *gameState;
}WorldView;