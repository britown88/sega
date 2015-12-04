#pragma once

#include "segautils\FSM.h"

typedef struct WorldView_t WorldView;
typedef struct Frame_t Frame;

DeclStateMessage(GameStateUpdate);
DeclStateMessage(GameStateHandleInput);
DeclStateMessageWithData(GameStateRender, { Frame *frame; });

//world state
StateClosure gameStateCreateWorld(WorldView *view);
StateClosure gameStateCreateConsole(WorldView *view);