#pragma once

#include "Time.h"

typedef struct FrameProfiler_t FrameProfiler;;

typedef enum {
   ProfileTimer_STEP,
   ProfileTimer_GAME,
   ProfileTimer_RENDER,
   ProfileTimer_POLLEVENTS,
   ProfileTimer_WAIT,
   ProfileTimer_COUNT
}FrameProfileTimer;

FrameProfiler *frameProfilerGet();

void frameProfilerStartTimer(FrameProfileTimer timer);
void frameProfilerEndTimer(FrameProfileTimer timer);

Microseconds frameProfilerGetResults(FrameProfileTimer timer);
void frameProfilerReset();





