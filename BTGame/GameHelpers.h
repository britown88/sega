#pragma once

#include "segautils/Vector.h"
#include "WorldView.h"

Int2 screenToWorld(WorldView *view, Int2 pos);

void changeBackground(WorldView *view, const char *file);