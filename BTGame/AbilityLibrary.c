#include "Abilities.h"
#include "segautils\BitTwiddling.h"

#define ClosureTPart CLOSURE_NAME(AbilityGenerator)
#include "segautils\Closure_Impl.h"

typedef struct {
   StringView key;
   AbilityGenerator value;
} agEntry;

#define HashTableT agEntry
#include "segautils\HashTable_Create.h"

struct AbilityLibrary_t {
   ht(agEntry) *table;
};

static int _agEntryCompare(agEntry *e1, agEntry *e2){
   return e1->key == e2->key;
}

static size_t _agEntryHash(agEntry *p){
   return hashPtr((void*)p->key);
}

static void _agEntryDestroy(agEntry *p){
   closureDestroy(AbilityGenerator)(&p->value);
}

AbilityLibrary *abilityLibraryCreate(){
   AbilityLibrary *out = checkedCalloc(1, sizeof(AbilityLibrary));
   out->table = htCreate(agEntry)(&_agEntryCompare, &_agEntryHash, &_agEntryDestroy);
   return out;
}

void abilityLibraryDestroy(AbilityLibrary *self){
   htDestroy(agEntry)(self->table);
   checkedFree(self);
}

void abilityLibraryAdd(AbilityLibrary *self, StringView name, AbilityGenerator c){
   agEntry entry = { .key = name, .value = c };
   htInsert(agEntry)(self->table, &entry);
}

AbilityGenerator abilityLibraryGet(AbilityLibrary *self, StringView name){
   agEntry entry = { 0 };
   agEntry *found = 0;
   AbilityGenerator out = { 0 };

   entry.key = name;
   found = htFind(agEntry)(self->table, &entry);

   if (found){
      out = found->value;
   }

   return out;
}