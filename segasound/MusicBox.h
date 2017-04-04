#pragma once

#include "segautils/Defs.h"

typedef struct MusicBox_t MusicBox;

MusicBox *musicBoxCreate();
void musicBoxDestroy(MusicBox *self);

void musicBoxTest(MusicBox *self);

bool musicBoxIsDone(MusicBox *self);
