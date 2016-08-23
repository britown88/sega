#include "GameClock.h"
#include "segautils\Defs.h"
#include "segashared\CheckedMemory.h"
#include "SEGA\App.h"

struct GameClock_t {
   Microseconds time, appTime;
   bool paused;
};

static GameClock g_clock = { 0 };
GameClock *gameClockGet() {
   return &g_clock;
}

static void _updateTime(GameClock *self) {
   Microseconds newTime = appGetTime(appGet());
   Microseconds elapsed = newTime - self->appTime;

   self->time += elapsed;
   self->appTime = newTime;
}

Microseconds gameClockGetTime(GameClock *self) {
   if (!self->paused) {
      _updateTime(self);
   }

   return self->time;
}
void gameClockPause(GameClock *self) {
   if (!self->paused) {
      _updateTime(self);
      self->paused = true;
   }
}
void gameClockResume(GameClock *self) {
   if (self->paused) {
      self->appTime = appGetTime(appGet());
      self->paused = false;
   }
}