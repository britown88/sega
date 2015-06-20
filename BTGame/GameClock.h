#pragma once

typedef struct GameClock_t GameClock;

GameClock *gameClockCreate();
void gameClockDestroy(GameClock *self);

long gameClockGetTime(GameClock *self);
void gameClockPause(GameClock *self);
void gameClockResume(GameClock *self);