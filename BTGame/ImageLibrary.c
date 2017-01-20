#include "ImageLibrary.h"
#include "DB.h"
#include "WorldView.h"
#include "assets.h"

#include "segalib\EGA.h"
#include "segautils\BitTwiddling.h"
#include "segautils/String.h"

typedef struct {
   StringView key;
   ManagedImage *value;
} iEntry;

#define HashTableT iEntry
#include "segautils\HashTable_Create.h"

struct ManagedImage_t{   
   ImageLibrary *parent;
   StringView name;
   Texture *texture;
   size_t useCount;
   bool loaded;
};

struct ImageLibrary_t {
   ht(iEntry) *table;
   WorldView *view;
};

void managedImageDestroy(ManagedImage *self){
   if (self->useCount > 1){
      --self->useCount;
   }
   else {
      iEntry entry = { self->name, self };
      htErase(iEntry)(self->parent->table, &entry);
   }
}
Texture *managedImageGetTexture(ManagedImage *self){

   if (!self->loaded) {
      DBImage img = dbImageSelectFirstByid(self->parent->view->db, self->name);
      Image *loadedImage;

      if (!img.image) {
         return NULL;
      }

      loadedImage = imageDeserialize(img.image, EGA_IMGD_OPTIMIZED);
      self->texture = imageCreateTexture(loadedImage);
      self->loaded = true;

      imageDestroy(loadedImage);
      dbImageDestroy(&img);
   }

   return self->texture;
}



static int _iEntryCompare(iEntry *e1, iEntry *e2){
   return e1->key == e2->key;
}

static size_t _iEntryHash(iEntry *p){
   return hashPtr((void*)p->key);
}

static void _iEntryDestroy(iEntry *p){
   if (p->value->loaded) {
      textureDestroy(p->value->texture);
   }   
   checkedFree(p->value);
}



ImageLibrary *imageLibraryCreate(WorldView *view){
   ImageLibrary *out = checkedCalloc(1, sizeof(ImageLibrary));
   out->view = view;
   out->table = htCreate(iEntry)(&_iEntryCompare, &_iEntryHash, &_iEntryDestroy);
   return out;
}
void imageLibraryDestroy(ImageLibrary *self){
   htDestroy(iEntry)(self->table);
   checkedFree(self);
}

static ManagedImage *_registerNew(ImageLibrary *self, StringView name) {   
   
   iEntry entry = { 0 };
   ManagedImage *out = NULL;


   //create a new entry
   out = checkedCalloc(1, sizeof(ManagedImage));
   out->parent = self;
   out->name = name;
   out->useCount = 1;
   out->loaded = false;

   entry.key = name;
   entry.value = out;

   htInsert(iEntry)(self->table, &entry);

   return out;
}


ManagedImage *imageLibraryGetImage(ImageLibrary *self, StringView name){
   iEntry entry = { 0 };
   iEntry *found = 0;
   
   entry.key = name;
   found = htFind(iEntry)(self->table, &entry);

   if (!found){
      return _registerNew(self, name);
   }

   ++found->value->useCount;
   return found->value;
}

void imageLibraryClear(ImageLibrary *self) {
   htForEach(iEntry, entry, self->table, {
      if (entry->value->loaded) {
         textureDestroy(entry->value->texture);
         entry->value->loaded = false;
      }
   });
}