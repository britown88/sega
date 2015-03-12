#include "CheckedMemory.h"
#include "Strings.h"
#include "segautils\Defs.h"
#include "segautils\IntrusiveHeap.h"
#include <stddef.h>
#include <stdio.h>

size_t hashPtr(void* ptr){
   size_t out = 5031;
   int i;
   for (i = 0; i < sizeof(void*); ++i) {
      out += (out << 5) + ((char*)&ptr)[i];
   }
   return out;
}

typedef struct {
   QueueNode node;
   StringView file;
   size_t line;
   size_t allocCount;   
} FileEntry;

FileEntry *_fileAllocCompareFunc(FileEntry *n1, FileEntry *n2){
   return n1->allocCount < n2->allocCount ? n1 : n2;
}
//priorityQueueCreate(offsetof(FileEntry, node), (PQCompareFunc)&_fileAllocCompareFunc);

#define HashTableT FileEntry
#include "segautils\HashTable_Create_Unchecked.h"

static int _fileEntryCompare(FileEntry *e1, FileEntry *e2){
   return e1->file == e1->file && e1->line == e2->line;
}

static size_t _fileEntryHash(FileEntry *file){
   size_t out = 5031;
   out += (out << 5) + file->line;
   out += (out << 5) + hashPtr((void*)file->file);
   return  (out << 5) + out;
}

static ht(FileEntry) *getFileTable(){
   static ht(FileEntry) *out = NULL;
   if (!out){
      out = htCreate(FileEntry)(&_fileEntryCompare, &_fileEntryHash, NULL);
   }
   return out;
}

static void freeFileTable(){
   htDestroy(FileEntry)(getFileTable());
}

typedef struct {
   StringView file;
   size_t line, bytes;
}AllocData;

typedef struct {
   void *key;
   AllocData value;
} adEntry;

#define HashTableT adEntry
#include "segautils\HashTable_Create_Unchecked.h"

static int _adEntryCompare(adEntry *e1, adEntry *e2){
   return e1->key == e2->key;
}

static size_t _adEntryHash(adEntry *ad){
   return hashPtr(ad->key);
}

static ht(adEntry) *getMemTable(){
   static ht(adEntry) *out = NULL;
   if (!out){
      out = htCreate(adEntry)(&_adEntryCompare, &_adEntryHash, NULL);
   }
   return out;
}

static void freeMemTable(){
   htDestroy(adEntry)(getMemTable());
}

static void addAlloc(StringView file, size_t line){
   ht(FileEntry)* rpt = getFileTable();
   FileEntry filerpt = { 0 };
   FileEntry *found;
   
   filerpt.allocCount = 0;
   filerpt.file = file;
   filerpt.line = line;
   
   found = htFind(FileEntry)(rpt, &filerpt);
   if (!found){
      ++filerpt.allocCount;
      htInsert(FileEntry)(rpt, &filerpt);
   }
   else{
      found->allocCount += 1;
   }
}

void* checkedMallocImpl(size_t sz, char* file, size_t line){
   StringView str = stringIntern(file);
   adEntry newEntry = { malloc(sz), { str, line, sz } };
   htInsert(adEntry)(getMemTable(), &newEntry);
   addAlloc(str, line);
   return newEntry.key;
}
void* checkedCallocImpl(size_t count, size_t sz, char* file, size_t line){
   StringView str = stringIntern(file);
   adEntry newEntry = { calloc(count, sz), { stringIntern(file), line, count*sz } };
   htInsert(adEntry)(getMemTable(), &newEntry);
   addAlloc(str, line);
   return newEntry.key;
}
void checkedFreeImpl(void* mem){
   adEntry e = { mem, { 0 } };
   
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
   FILE *output = NULL;

   output = fopen("allocReport.csv", "wt");
   fprintf(output, "File,Line,Alloc Count\n");
   htForEach(FileEntry, e, getFileTable(), {
      fprintf(output, "%s,%i,%i\n", e->file, e->line, e->allocCount);
   });
   fclose(output);

   output = fopen("memleak.txt", "wt");
   fprintf(output, "MEMORY LEAKS\n");
   fprintf(output, "-------------START---------------\n");
   htForEach(adEntry, e, getMemTable(), {
      AllocData *data = &e->value;
      fprintf(output, "%i bytes in %s:%i\n", data->bytes, data->file, data->line);
   });
   fprintf(output, "-------------END-----------------\n");

   fclose(output);
   freeMemTable();
   freeFileTable();
#endif
}