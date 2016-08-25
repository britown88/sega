#pragma once

#include "segashared/Strings.h"
#include "segautils/Vector.h"
#include "GameClock.h"

typedef struct WorldView_t WorldView;
typedef struct Frame_t Frame;
typedef struct FrameRegion_t FrameRegion;

typedef struct Sprite_t Sprite;

void spriteDestroy(Sprite *self);//destroy in-memory object

void spriteSetAnimationSpeed(Sprite *self, Milliseconds timePerFrame);
void spriteSetRepeat(Sprite *self, bool repeat);//default repeat is true
void spriteReset(Sprite *self);//resets the animation
void frameRenderSprite(Frame *frame, FrameRegion *vp, short x, short y, Sprite *sprite);

typedef struct SpriteManager_t SpriteManager;

SpriteManager *spriteManagerCreate(WorldView *view);
void spriteManagerDestroy(SpriteManager *self);
Sprite *spriteManagerGetSprite(SpriteManager *self, StringView id);

void spriteManagerClear(SpriteManager *self);

