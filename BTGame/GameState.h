#pragma once

#include "segautils\FSM.h"

typedef struct WorldView_t WorldView;
typedef struct Frame_t Frame;

DeclStateMessage(GameStateUpdate);
DeclStateMessage(GameStateHandleInput);
DeclStateMessageWithData(GameStateRender, { Frame *frame; });

//world state
StateClosure gameStateCreateWorld(WorldView *view);
#define STARTING_AMBIENT_LEVEL MAX_BRIGHTNESS
typedef struct {
   WorldView *view;
}WorldState;
void worldStateHandleKeyboardConsole(WorldState *state);
void worldStateHandleMouseConsole(WorldState *state);
void worldStateHandleKeyboard(WorldState *state);
void worldStateHandleMouse(WorldState *state);