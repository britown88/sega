#pragma once

#include "segautils/Time.h"

typedef struct GameClock_t GameClock;


Microseconds gameClockGetTime();
void gameClockPause(GameClock *self);
void gameClockResume(GameClock *self);

GameClock *gameClockGet();