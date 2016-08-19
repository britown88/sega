#include "Sprites.h"
#include "DB.h"
#include "segalib/EGA.h"
#include "segashared/CheckedMemory.h"

struct Sprite_t {
   int i;
};

Sprite *spriteGet(DB *db, StringView id) {
}
void spriteDestroy(Sprite *self) {
}

void spriteSetAnimationSpeed(Sprite *self, Milliseconds timePerFrame) {
}
void spriteSetRepeat(Sprite *self, bool repeat) {

}
void spriteRender(Sprite *self, Frame *frame) {

}