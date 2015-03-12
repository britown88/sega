#include "CheckedMemory.h"
#include "Strings.h"
#include "segautils\Defs.h"
#include <stddef.h>
#include <stdio.h>

//in a .cpp file
typedef struct 
{
   StringView file;
   size_t line, bytes;
}AllocData;

typedef struct {
   void *key;
   AllocData *value;
} adEntry;

static AllocData *allocDataCreate(StringView file, size_t line, size_t bytes){
   AllocData *out = calloc(1, sizeof(AllocData));
   out->bytes = bytes;
   out->line = line;
   out->file = file;
   return out;
}

static void allocDataDestroy(AllocData *self){
   free(self);
}

#define HashTableT adEntry
#include "segautils\HashTable_Create_Unchecked.h"

static int _adEntryCompare(adEntry *e1, adEntry *e2){
   return e1->key == e2->key;
}

static size_t _adEntryHash(adEntry *ad){
   return (size_t)ad->key;
}

static void _adEntryDestroy(adEntry *ad){
   allocDataDestroy(ad->value);
}

static ht(adEntry) *getMemTable(){
   static ht(adEntry) *out = NULL;
   if (!out){
      out = htCreate(adEntry)(&_adEntryCompare, &_adEntryHash, &_adEntryDestroy);
   }
   return out;
}

static void freeMemTable(){
   htDestroy(adEntry)(getMemTable());
}

void* checkedMallocImpl(size_t sz, char* file, size_t line){
   adEntry newEntry = { malloc(sz), allocDataCreate(stringIntern(file), line, sz) };
   htInsert(adEntry)(getMemTable(), &newEntry);
   return newEntry.key;
}
void* checkedCallocImpl(size_t count, size_t sz, char* file, size_t line){
   adEntry newEntry = { calloc(count, sz), allocDataCreate(stringIntern(file), line, count*sz) };
   htInsert(adEntry)(getMemTable(), &newEntry);
   return newEntry.key;
}
void checkedFreeImpl(void* mem){
   adEntry e = { mem, NULL };
   
   if (!mem){ return; }
   htErase(adEntry)(getMemTable(), htFind(adEntry)(getMemTable(), &e));
   free(mem);
}
void* uncheckedMallocImpl(size_t sz, char* file, size_t line){
}

void* uncheckedCallocImpl(size_t count, size_t sz, char* file, size_t line){
}

void printMemoryLeaks(){
#ifdef _DEBUG
   FILE *output = fopen("memleak.txt", "wt");

   htForEach(adEntry, e, getMemTable(), {
      AllocData *data = e->value;
      fprintf(output, "%i bytes in %s:%i\n", data->bytes, data->file, data->line);
   });

   fclose(output);
   freeMemTable();
#endif
}