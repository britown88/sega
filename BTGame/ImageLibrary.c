#include "ImageLibrary.h"
#include "DB.h"
#include "WorldView.h"

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
   ht(iEntry) *map;
   StringView name;
   String *path;
   Image *image;
   size_t useCount;
};

void managedImageDestroy(ManagedImage *self){
   if (self->useCount > 1){
      --self->useCount;
   }
   else {
      iEntry entry = { self->name, self };
      htErase(iEntry)(self->map, &entry);
   }
}
Image *managedImageGetImage(ManagedImage *self){
   return self->image;
}



static int _iEntryCompare(iEntry *e1, iEntry *e2){
   return e1->key == e2->key;
}

static size_t _iEntryHash(iEntry *p){
   return hashPtr((void*)p->key);
}

static void _iEntryDestroy(iEntry *p){
   imageDestroy(p->value->image);
   stringDestroy(p->value->path);
   checkedFree(p->value);
}

struct ImageLibrary_t{
   ht(iEntry) *table;
   WorldView *view;
};

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

static int _registerBuffer(ImageLibrary *self, StringView name, byte *buffer, int size) {   
   
   iEntry entry = { 0 };
   ManagedImage *out = NULL;
   Image *newImage = imageDeserializeOptimizedFromBuffer(buffer, size);

   if (!newImage) {
      //failed to laod image, get fucked
      return 1;
   }

   entry.key = name;

   //create a new entry
   entry.value = checkedCalloc(1, sizeof(ManagedImage));
   entry.value->image = newImage;
   entry.value->map = self->table;
   //entry.value->path = stringCreate(assetPath);
   entry.value->name = name;
   entry.value->useCount = 1;

   htInsert(iEntry)(self->table, &entry);

   return 0;
}


ManagedImage *imageLibraryGetImage(ImageLibrary *self, StringView name){
   iEntry entry = { 0 };
   iEntry *found = 0;
   
   entry.key = name;
   found = htFind(iEntry)(self->table, &entry);

   if (!found){  
      byte *buffer;
      int bSize;
      int result = DBSelectImage(self->view->db, name, &buffer, &bSize);

      if (result) {
         return NULL;
      }

      result = _registerBuffer(self, name, buffer, bSize);
      if (result) {
         return NULL;
      }

      return imageLibraryGetImage(self, name);
   }

   ++found->value->useCount;
   return found->value;
}

int imageLibraryRegisterName(ImageLibrary *self, StringView name, const char *assetPath) {
   iEntry entry = { 0 };
   iEntry *found = 0;

   Image *newImage = imageDeserializeOptimized(assetPath);

   if (!newImage) {
      //failed to laod image, get fucked
      return 1;
   }

   entry.key = name;
   found = htFind(iEntry)(self->table, &entry);

   if (found) {
      //kill the old image
      imageDestroy(found->value->image);

      stringSet(found->value->path, assetPath);      
      found->value->image = newImage;      
   }
   else {
      //create a new entry
      entry.value = checkedCalloc(1, sizeof(ManagedImage));
      entry.value->image = newImage;
      entry.value->map = self->table;
      entry.value->path = stringCreate(assetPath);
      entry.value->name = name;
      entry.value->useCount = 1;

      htInsert(iEntry)(self->table, &entry);
   }

   return 0;
}