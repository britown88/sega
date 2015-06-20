#include "StatusManager.h"
#include "segautils\BitTwiddling.h"

#define ClosureTPart CLOSURE_NAME(StatusGenerator)
#include "segautils\Closure_Impl.h"

typedef struct {
   StringView key;
   StatusGenerator value;
} stEntry;

#define HashTableT stEntry
#include "segautils\HashTable_Create.h"

struct StatusLibrary_t {
   ht(stEntry) *table;
};

static int _stEntryCompare(stEntry *e1, stEntry *e2){
   return e1->key == e2->key;
}

static size_t _stEntryHash(stEntry *p){
   return hashPtr((void*)p->key);
}

static void _stEntryDestroy(stEntry *p){
   closureDestroy(StatusGenerator)(&p->value);
}

StatusLibrary *statusLibraryCreate(){
   StatusLibrary *out = checkedCalloc(1, sizeof(StatusLibrary));
   out->table = htCreate(stEntry)(&_stEntryCompare, &_stEntryHash, &_stEntryDestroy);
   return out;
}

void statusLibraryDestroy(StatusLibrary *self){
   htDestroy(stEntry)(self->table);
   checkedFree(self);
}

void statusLibraryAdd(StatusLibrary *self, StringView name, StatusGenerator c){
   stEntry entry = { .key = name, .value = c };
   htInsert(stEntry)(self->table, &entry);
}

StatusGenerator statusLibraryGet(StatusLibrary *self, StringView name){
   stEntry entry = { 0 };
   stEntry *found = 0;
   StatusGenerator out = { 0 };

   entry.key = name;
   found = htFind(stEntry)(self->table, &entry);

   if (found){
      out = found->value;
   }

   return out;
}