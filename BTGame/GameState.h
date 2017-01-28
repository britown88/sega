#pragma once

#include "segautils\FSM.h"

typedef struct WorldView_t WorldView;
typedef struct Texture_t Texture;

DeclStateMessage(GameStateUpdate);
DeclStateMessage(GameStateHandleInput);
DeclStateMessage(GameStateOpenMapEditor);
DeclStateMessageWithData(GameStateRender, { Texture *frame; });

//world state
StateClosure gameStateCreateWorld(WorldView *view);
StateClosure gameStateCreateConsole(WorldView *view);
StateClosure gameStateCreateEditor(WorldView *view);
StateClosure gameStateCreateSplash(WorldView *view);

//giantrom
StateClosure gameStateCreateGBSplash(WorldView *view);
