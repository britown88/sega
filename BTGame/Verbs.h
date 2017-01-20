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

typedef struct Texture_t Texture;

VerbManager *verbManagerCreate(WorldView *view);
void verbManagerDestroy(VerbManager *self);

void verbManagerSetEnabled(VerbManager *self, bool enabled);

//returns nonzero if event occurs
int verbManagerMouseButton(VerbManager *self, MouseEvent *e);
void verbManagerKeyButton(VerbManager *self, Verbs v, SegaKeyActions action);

void verbManagerRender(VerbManager *self, Texture *tex);