#include "FrameProfiler.h"
#include "SEGA/App.h"
#include "segashared/CheckedMemory.h"

struct FrameProfiler_t {
   Microseconds startTimes[ProfileTimer_COUNT];
   Microseconds results[ProfileTimer_COUNT];
};

FrameProfiler *frameProfilerGet() {
   static FrameProfiler *out = NULL;
   if (!out) {
      out = calloc(1, sizeof(FrameProfiler));
   }
   return out;
}

void frameProfilerStartTimer(FrameProfileTimer timer) {
   FrameProfiler *self = frameProfilerGet();
   self->startTimes[timer] = appGetTime(appGet());
}
void frameProfilerEndTimer(FrameProfileTimer timer) {
   FrameProfiler *self = frameProfilerGet();
   self->results[timer] = appGetTime(appGet()) - self->startTimes[timer];
}

Microseconds frameProfilerGetResults(FrameProfileTimer timer) {
   FrameProfiler *self = frameProfilerGet();
   return self->results[timer];
}
void frameProfilerReset() {
   FrameProfiler *self = frameProfilerGet();
   memset(self->startTimes, 0, sizeof(self->startTimes));
   memset(self->results, 0, sizeof(self->results));
}