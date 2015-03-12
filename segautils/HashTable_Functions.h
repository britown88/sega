#include "Preprocessor.h"

#ifndef ht
#define ht(TYPE) CONCAT(ht_, TYPE)
#define htCreate(TYPE) CONCAT(htCreate_, TYPE)
#define htDestroy(TYPE) CONCAT(htDestroy_, TYPE)
#define htInsert(TYPE) CONCAT(htInsert_, TYPE)
#define htErase(TYPE) CONCAT(htErase_, TYPE)
#define htFind(TYPE) CONCAT(htFind_, TYPE)
#define _htGrow(TYPE) CONCAT(_htGrow_, TYPE)

#define htForEach(TYPE, varName, list, ...) \
if(list->buckets) { \
   CONCAT(htbucket_, TYPE) **bucket = list->buckets; \
   CONCAT(htbucket_, TYPE) **end = bucket + (1 << list->power); \
   for (; bucket != end; ++bucket){ \
      CONCAT(htbucket_, TYPE) *iter = *bucket; \
      while (iter){ \
         TYPE *varName = &iter->data; \
         __VA_ARGS__ \
         iter = iter->next; \
      } \
   } \
}

#endif