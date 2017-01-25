#pragma once

#include "SEGA\App.h"
#include "segautils\DLLBullshit.h"
#include "segautils/extern_c.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define FULLSCREEN 0
#define FRAME_RATE 60


SEXTERN_C
VirtualApp *btCreate();
END_SEXTERN_C
