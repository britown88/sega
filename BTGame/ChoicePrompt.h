#pragma once

#include "segautils/StandardVectors.h"
#include "WorldView.h"
#include "SEGA/Input.h"

typedef struct ChoicePrompt_t ChoicePrompt;

ChoicePrompt *createChoicePrompt(WorldView *view);
void choicePromptDestroy(ChoicePrompt *self);

void choicePromptSetChoices(ChoicePrompt *self, const char **choices);
vec(StringPtr) *choicePromptGetChoices(ChoicePrompt *self);
int choicePromptGetDecisionIndex(ChoicePrompt *self);
const char *choicePromptGetDecision(ChoicePrompt *self);