#include "Sprites.h"
#include "assets.h"
#include "ImageLibrary.h"
#include "segalib/EGA.h"
#include "segashared/CheckedMemory.h"
#include "segashared/Strings.h"
#include "WorldView.h"

typedef struct {
   Int2 imgPos;
   ManagedImage *img;   
}SpriteFrame;

#define VectorT SpriteFrame
#include "segautils/Vector_Create.h"

void spriteFrameDestroy(SpriteFrame *self) {
   if (self->img) {
      managedImageDestroy(self->img);
   }
}

struct Sprite_t {
   Int2 size;
   vec(SpriteFrame) *frames;
   bool repeat;
   Milliseconds msPerFrame;
   Microseconds startTime;
};

Sprite *spriteGet(WorldView *view, StringView id) {
   Sprite *out = NULL;
   vec(DBSpriteFrame) *frames = NULL;
   DBSprite dbs = dbSpriteSelectFirstByid(view->db, id);

   if (!dbs.id) {
      return out;
   }

   if (frames = dbSpriteFrameSelectBysprite(view->db, id)) {
      size_t i = 0;
      size_t fCount = vecSize(DBSpriteFrame)(frames);
      out = checkedCalloc(1, sizeof(Sprite));
      out->frames = vecCreate(SpriteFrame)(&spriteFrameDestroy);
      out->repeat = true;
      out->msPerFrame = 250;

      out->size.x = dbs.width;
      out->size.y = dbs.height;

      vecResize(SpriteFrame)(out->frames, fCount, &(SpriteFrame){0});

      for (i = 0; i < fCount; ++i) {
         DBSpriteFrame *f = vecAt(DBSpriteFrame)(frames, i);
         SpriteFrame *newFrame = vecAt(SpriteFrame)(out->frames, f->index);

         newFrame->imgPos.x = f->imgX;
         newFrame->imgPos.y = f->imgY;

         newFrame->img = imageLibraryGetImage(
            view->imageLibrary, 
            stringIntern(c_str(f->image)));
      }

      vecDestroy(DBSpriteFrame)(frames);
   }
   
   dbSpriteDestroy(&dbs);
   return out;
}

void spriteDestroy(Sprite *self) {
   vecDestroy(SpriteFrame)(self->frames);
   checkedFree(self);
}

void spriteSetAnimationSpeed(Sprite *self, Milliseconds timePerFrame) {
   self->msPerFrame = timePerFrame;
}

void spriteSetRepeat(Sprite *self, bool repeat) {
   self->repeat = repeat;
}

void spriteReset(Sprite *self) {
   self->startTime = 0;
}

static SpriteFrame *_getFrame(Sprite *self) {
   Milliseconds delta = 0;
   size_t index = 0;
   size_t fCount = vecSize(SpriteFrame)(self->frames);

   if (!self->startTime) {
      self->startTime = gameClockGetTime();
   }

   delta = t_u2m(gameClockGetTime() - self->startTime);

   if (!self->repeat && delta >= fCount * self->msPerFrame) {
      index = fCount - 1;
   }
   else {
      index = (delta / self->msPerFrame) % fCount;
   }

   return vecAt(SpriteFrame)(self->frames, index);
}

void frameRenderSprite(Frame *frame, FrameRegion *vp, short x, short y, Sprite *sprite) {
   SpriteFrame *f = _getFrame(sprite);
   frameRenderImagePartial(frame, vp, x, y, managedImageGetImage(f->img), 
      f->imgPos.x * sprite->size.x, f->imgPos.y * sprite->size.x, 
      sprite->size.x, sprite->size.y);
}