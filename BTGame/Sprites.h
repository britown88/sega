#pragma once

#include "segashared/Strings.h"
#include "GameClock.h"

typedef struct DB_t DB;
typedef struct Frame_t Frame;

typedef struct Sprite_t Sprite;

Sprite *spriteGet(DB *db, StringView id);//get sprite from database
void spriteDestroy(Sprite *self);//destroy in-memory object

void spriteSetAnimationSpeed(Sprite *self, Milliseconds timePerFrame);
void spriteSetRepeat(Sprite *self, bool repeat);//default repeat is true
void spriteRender(Sprite *self, Frame *frame);

