#pragma once

#include "segautils/Time.h"

typedef struct GameClock_t GameClock;

GameClock *gameClockCreate();
void gameClockDestroy(GameClock *self);

Microseconds gameClockGetTime(GameClock *self);
void gameClockPause(GameClock *self);
void gameClockResume(GameClock *self);