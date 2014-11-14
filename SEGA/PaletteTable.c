#include "PaletteTable.h"
#include "segalib\EGA.h"
#include "bt-utils\CheckedMemory.h"

#include <string.h>

typedef struct {
   Palette key;
   EGAPalette *value;
} pEntry;

#define T pEntry
#include "bt-utils\HashTable_Create.h"

static int _pEntryCompare(pEntry *e1, pEntry *e2){
   return !memcmp(e1->key.colors, e2->key.colors, EGA_PALETTE_COLORS);
}

static size_t _pEntryHash(pEntry *p){
   size_t out = 5031;

   int i;
   for(i = 0; i < EGA_PALETTE_COLORS; ++i){
      out = (out << 5) + out + p->key.colors[i];
   }

   return out;
}

static void _pEntryDestroy(pEntry *p){
   egaPaletteDestroy(p->value);
}

struct PaletteTable_t{
   ht(pEntry) *table;
};

PaletteTable *paletteTableCreate(){
   PaletteTable *r = checkedCalloc(1, sizeof(PaletteTable));
   r->table = htCreate(pEntry)(&_pEntryCompare, &_pEntryHash, &_pEntryDestroy);
   return r;
}
void paletteTableDestroy(PaletteTable *self){
   htDestroy(pEntry)(self->table);
   checkedFree(self);
}

EGAPalette *paletteTableGetPalette(PaletteTable *self, byte *palette){
   Palette p = {0};
   pEntry entry = {0};
   pEntry *found = 0;
   EGAPalette *ret = 0;

   memcpy(p.colors, palette, EGA_PALETTE_COLORS);
   entry.key = p;

   found = htFind(pEntry)(self->table, &entry);

   if(!found){
      ret = egaPaletteCreate(palette);
      entry.value = ret;
      htInsert(pEntry)(self->table, &entry);
   }
   else {
      ret = found->value;
   }

   return ret;
}