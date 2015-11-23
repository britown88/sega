#pragma once

#include "WorldView.h"
#include "SEGA/Input.h"

typedef enum {
   Verb_Look = 0,
   Verb_Use,
   Verb_Talk,   
   Verb_Fight,
   Verb_COUNT
} Verbs;

typedef struct VerbManager_t VerbManager;

VerbManager *createVerbManager(WorldView *view);
void verbManagerCreateVerbs(VerbManager *self);

//returns nonzero if event occurs
int verbManagerMouseButton(VerbManager *self, MouseEvent *e);

void verbManagerKeyButton(VerbManager *self, Verbs v, SegaKeyActions action);