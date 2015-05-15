#include "CombatRoutines.h"
#include "segautils\BitTwiddling.h"

typedef struct {
   StringView key;
   Coroutine value;
} crEntry;

#define HashTableT crEntry
#include "segautils\HashTable_Create.h"

struct CombatRoutineLibrary_t {
   ht(crEntry) *table;
};

static int _crEntryCompare(crEntry *e1, crEntry *e2){
   return e1->key == e2->key;
}

static size_t _crEntryHash(crEntry *p){
   return hashPtr((void*)p->key);
}

static void _crEntryDestroy(crEntry *p){
   closureDestroy(Coroutine)(&p->value);
}

CombatRoutineLibrary *combatRoutineLibraryCreate(){
   CombatRoutineLibrary *out = checkedCalloc(1, sizeof(CombatRoutineLibrary));
   out->table = htCreate(crEntry)(&_crEntryCompare, &_crEntryHash, &_crEntryDestroy);
   return out;
}

void combatRoutineLibraryDestroy(CombatRoutineLibrary *self){
   htDestroy(crEntry)(self->table);
   checkedFree(self);
}

void combatRoutineLibraryAdd(CombatRoutineLibrary *self, StringView name, Coroutine c){
   crEntry entry = { .key = name, .value = c };
   htInsert(crEntry)(self->table, &entry);
}

Coroutine combatRoutineLibraryGet(CombatRoutineLibrary *self, StringView name){
   crEntry entry = { 0 };
   crEntry *found = 0;
   Coroutine out = coroutineNull();

   entry.key = name;
   found = htFind(crEntry)(self->table, &entry);

   if (found){
      out = found->value; 
   }

   return out;
}