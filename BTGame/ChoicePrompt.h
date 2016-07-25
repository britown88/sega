#pragma once

#include "segautils/StandardVectors.h"
#include "SEGA/Input.h"
#include "WorldView.h"
#include "SEGA/Input.h"

typedef struct ChoicePrompt_t ChoicePrompt;

ChoicePrompt *createChoicePrompt(WorldView *view);
void choicePromptDestroy(ChoicePrompt *self);

void choicePromptUpdate(ChoicePrompt *self);

//assumes ownership of the vector
void choicePromptSetChoices(ChoicePrompt *self, vec(StringPtr) *choices);

//returns nonzero if event occurs
int choicePromptHandleMouseEvent(ChoicePrompt *self, MouseEvent *e);

//returns nonzero if event occurs
int choicePromptHandleKeyEvent(ChoicePrompt *self, KeyboardEvent *e);

//returns NULL if no selection is yet made
const char *choicePromptGetDecision(ChoicePrompt *self);