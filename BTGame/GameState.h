#pragma once

#include "segautils\FSM.h"

typedef struct WorldView_t WorldView;
typedef struct Frame_t Frame;

DeclStateMessage(GameStateUpdate);
DeclStateMessage(GameStateHandleInput);
DeclStateMessage(GameStateOpenMapEditor);
DeclStateMessageWithData(GameStateRender, { Frame *frame; });

//world state
StateClosure gameStateCreateWorld(WorldView *view);
StateClosure gameStateCreateConsole(WorldView *view);
StateClosure gameStateCreateEditor(WorldView *view);
StateClosure gameStateCreateSplash(WorldView *view);