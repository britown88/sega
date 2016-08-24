#pragma once

#include "Viewport.h"

typedef struct ImageLibrary_t ImageLibrary;
typedef struct FSM_t FSM;
typedef struct GameClock_t GameClock;
typedef struct GridSolver_t GridSolver;
typedef struct Console_t Console;
typedef struct lua_State lua_State;
typedef struct ChoicePrompt_t ChoicePrompt;
typedef struct DB_assets DB_assets;
typedef struct Weather_t Weather;
typedef struct TextAreaManager_t TextAreaManager;

typedef struct RenderManager_t RenderManager;
typedef struct CursorManager_t CursorManager;
typedef struct GridManager_t GridManager;
typedef struct PCManager_t PCManager;
typedef struct VerbManager_t VerbManager;
typedef struct ActorManager_t ActorManager;
typedef struct ClockManager_t ClockManager;

typedef struct FramerateViewer_t FramerateViewer;

typedef struct WorldView_t {
   

   CursorManager *cursorManager;
   GridManager *gridManager;
   PCManager *pcManager;
   VerbManager *verbManager;
   ActorManager *actorManager;
   TextAreaManager *textAreaManager;

   FramerateViewer *framerateViewer;
   FontFactory *fontFactory;
   ImageLibrary *imageLibrary;
   FSM *gameState;
   GameClock *gameClock;
   GridSolver *gridSolver;
   Viewport *viewport;
   Console *console;
   lua_State *L;
   ChoicePrompt *choicePrompt;
   DB_assets *db;
   Weather *weather;
   
}WorldView;