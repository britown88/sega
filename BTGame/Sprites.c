#include "Sprites.h"
#include "assets.h"
#include "ImageLibrary.h"
#include "segalib/EGA.h"
#include "segashared/CheckedMemory.h"
#include "segashared/Strings.h"
#include "WorldView.h"

#define GLOBAL_ANIMATION_SPEED 1000

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
   bool repeat, attached;
   Milliseconds msPerFrame;
   Microseconds startTime;

   bool loaded;
   StringView id;
   size_t useCount;
   SpriteManager *parent;
};

static void _loadSprite(Sprite *self);
static void _unloadSprite(Sprite *self);

typedef struct {
   StringView key;
   Sprite *value;
} SpriteEntry;

#define HashTableT SpriteEntry
#include "segautils\HashTable_Create.h"

static int _SpriteEntryCompare(SpriteEntry *e1, SpriteEntry *e2) {
   return e1->key == e2->key;
}

static size_t _SpriteEntryyHash(SpriteEntry *p) {
   return hashPtr((void*)p->key);
}

static void _SpriteEntryDestroy(SpriteEntry *p) {
   vecDestroy(SpriteFrame)(p->value->frames);
   checkedFree(p->value);
}

void spriteAttachToGlobalSpeed(Sprite *self) {
   self->attached = true;
}
void spriteDetachFromGlobalSpeed(Sprite *self) {
   self->attached = false;
}


struct SpriteManager_t {
   WorldView *view;
   ht(SpriteEntry) *sprites;

   Microseconds globalStartTime;
};

SpriteManager *spriteManagerCreate(WorldView *view) {
   SpriteManager *out = checkedCalloc(1, sizeof(SpriteManager));
   out->view = view;
   out->sprites = htCreate(SpriteEntry)(&_SpriteEntryCompare, &_SpriteEntryyHash, &_SpriteEntryDestroy);
   return out;
}
void spriteManagerDestroy(SpriteManager *self) {
   htDestroy(SpriteEntry)(self->sprites);
   checkedFree(self);
}

void spriteManagerClear(SpriteManager *self) {
   htForEach(SpriteEntry, e, self->sprites, {
      _unloadSprite(e->value);
   });
}

static Sprite *_registerNewSprite(SpriteManager *self, StringView id) {
   SpriteEntry entry = { 0 };
   Sprite *out = out = checkedCalloc(1, sizeof(Sprite));

   out->frames = vecCreate(SpriteFrame)(&spriteFrameDestroy);
   out->parent = self;
   out->repeat = true;
   out->msPerFrame = 250;
   out->loaded = false;
   out->useCount = 1;
   out->id = id;

   entry.key = id;
   entry.value = out;

   htInsert(SpriteEntry)(self->sprites, &entry);

   return out;
}


Sprite *spriteManagerGetSprite(SpriteManager *self, StringView id) {
   SpriteEntry entry = { 0 };
   SpriteEntry *found = NULL;

   entry.key = id;
   found = htFind(SpriteEntry)(self->sprites, &entry);

   if (!found) {
      return _registerNewSprite(self, id);
   }

   ++found->value->useCount;
   return found->value;
}

void spriteDestroy(Sprite *self) {
   if (self->useCount > 1) {
      --self->useCount;
   }
   else {
      SpriteEntry entry = { self->id, self };
      htErase(SpriteEntry)(self->parent->sprites, &entry);
   }
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


void _unloadSprite(Sprite *self) {
   if (!self->loaded) {
      return;
   }

   vecClear(SpriteFrame)(self->frames);
   self->loaded = false;
}

void _loadSprite(Sprite *self) {
   WorldView *view = self->parent->view;
   vec(DBSpriteFrame) *frames = NULL;
   DBSprite dbs = { 0 };
   
   if (self->loaded) {
      return;
   }
   
   dbs = dbSpriteSelectFirstByid(view->db, self->id);

   if (!dbs.id) {
      return;
   }

   if (frames = dbSpriteFrameSelectBysprite(view->db, self->id)) {
      size_t i = 0;
      size_t fCount = vecSize(DBSpriteFrame)(frames);      

      self->size.x = dbs.width;
      self->size.y = dbs.height;

      vecResize(SpriteFrame)(self->frames, fCount, &(SpriteFrame){0});

      for (i = 0; i < fCount; ++i) {
         DBSpriteFrame *f = vecAt(DBSpriteFrame)(frames, i);
         SpriteFrame *newFrame = vecAt(SpriteFrame)(self->frames, f->index);

         newFrame->imgPos.x = f->imgX;
         newFrame->imgPos.y = f->imgY;

         newFrame->img = imageLibraryGetImage(
            view->imageLibrary,
            stringIntern(c_str(f->image)));
      }

      vecDestroy(DBSpriteFrame)(frames);
      self->loaded = true;
   }

   dbSpriteDestroy(&dbs);
}

static SpriteFrame *_getFrame(Sprite *self) {
   Milliseconds delta = 0;
   size_t index = 0;
   size_t fCount = vecSize(SpriteFrame)(self->frames);
   Microseconds startTime = 0;
   Milliseconds msPerFrame = 0;

   if (!fCount) {
      return NULL;
   }

   if (!self->startTime) {
      self->startTime = gameClockGetTime();
   }

   startTime = self->attached ? self->parent->globalStartTime : self->startTime;
   msPerFrame = self->attached ? GLOBAL_ANIMATION_SPEED : self->msPerFrame;

   delta = t_u2m(gameClockGetTime() - startTime);

   if (!self->repeat && delta >= fCount * msPerFrame) {
      index = fCount - 1;
   }
   else {
      index = (delta / msPerFrame) % fCount;
   }

   return vecAt(SpriteFrame)(self->frames, index);
}

void frameRenderSprite(Frame *frame, FrameRegion *vp, short x, short y, Sprite *sprite) {
   
   
   if (sprite) {
      SpriteFrame *f = NULL;
      _loadSprite(sprite);
      f = _getFrame(sprite);

      if (f) {
         frameRenderImagePartial(frame, vp, x, y, managedImageGetImage(f->img),
            f->imgPos.x * sprite->size.x, f->imgPos.y * sprite->size.y,
            sprite->size.x, sprite->size.y);
      }
   }

   
}