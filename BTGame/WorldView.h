#pragma once

#include "Viewport.h"

typedef struct BTManagers_t BTManagers;
typedef struct EntitySystem_t EntitySystem;
typedef struct ImageLibrary_t ImageLibrary;
typedef struct FSM_t FSM;
typedef struct GameClock_t GameClock;

typedef struct WorldView_t {
   BTManagers *managers;
   EntitySystem *entitySystem;
   ImageLibrary *imageLibrary;
   FSM *gameState;
   GameClock *gameClock;
   Viewport viewport;
}WorldView;