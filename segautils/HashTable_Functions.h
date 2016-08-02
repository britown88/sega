#include "Preprocessor.h"

#ifndef ht
#define ht(TYPE) CONCAT(ht_, TYPE)
#define htCreate(TYPE) CONCAT(htCreate_, TYPE)
#define htClear(TYPE) CONCAT(htClear_, TYPE)
#define htDestroy(TYPE) CONCAT(htDestroy_, TYPE)
#define htInsert(TYPE) CONCAT(htInsert_, TYPE)
#define htErase(TYPE) CONCAT(htErase_, TYPE)
#define htFind(TYPE) CONCAT(htFind_, TYPE)
#define _htGrow(TYPE) CONCAT(_htGrow_, TYPE)

#define htForEach(TYPE, varName, hashTable, ...) { \
ht(TYPE) *CONCAT(__temp_, ht(TYPE)) =  hashTable;\
if(CONCAT(__temp_, ht(TYPE))->buckets) { \
   CONCAT(htbucket_, TYPE) **bucket = CONCAT(__temp_, ht(TYPE))->buckets; \
   CONCAT(htbucket_, TYPE) **end = bucket + (1 << CONCAT(__temp_, ht(TYPE))->power); \
   for (; bucket != end; ++bucket){ \
      CONCAT(htbucket_, TYPE) *iter = *bucket; \
      while (iter){ \
         TYPE *varName = &iter->data; \
         __VA_ARGS__ \
         iter = iter->next; \
      } \
   } \
}}

#endif