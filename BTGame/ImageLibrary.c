#include "ImageLibrary.h"

#include "segalib\EGA.h"
#include "segautils\BitTwiddling.h"

typedef struct {
   StringView key;
   ManagedImage *value;
} iEntry;

#define HashTableT iEntry
#include "segautils\HashTable_Create.h"

typedef struct ManagedImage_t{   
   ht(iEntry) *map;
   StringView path;
   Image *image;
   size_t useCount;
};

void managedImageDestroy(ManagedImage *self){
   if (self->useCount > 1){
      --self->useCount;
   }
   else {
      iEntry entry = { self->path, self };
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
   checkedFree(p->value);
}

typedef struct ImageLibrary_t{
   ht(iEntry) *table;
};

ImageLibrary *imageLibraryCreate(){
   ImageLibrary *out = checkedCalloc(1, sizeof(ImageLibrary));
   out->table = htCreate(iEntry)(&_iEntryCompare, &_iEntryHash, &_iEntryDestroy);
   return out;
}
void imageLibraryDestroy(ImageLibrary *self){
   htDestroy(iEntry)(self->table);
   checkedFree(self);
}
ManagedImage *imageLibraryGetImage(ImageLibrary *self, StringView path){
   iEntry entry = { 0 };
   iEntry *found = 0;
   ManagedImage *out = 0;
   
   entry.key = path;
   found = htFind(iEntry)(self->table, &entry);

   if (!found){      
      Image *newImage = imageDeserializeOptimized(path);
      
      if (newImage){
         out = checkedCalloc(1, sizeof(ManagedImage));
         out->image = newImage;
         out->map = self->table;
         out->path = path;
         out->useCount = 1;

         entry.value = out;
         htInsert(iEntry)(self->table, &entry);
      }
   }
   else {
      ++found->value->useCount;
      out = found->value;
   }

   return out;
}