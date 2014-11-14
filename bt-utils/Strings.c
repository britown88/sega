#include "Strings.h"

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "CheckedMemory.h"
 
#define NAME_BUCKET_EXPONENT 16
 
typedef struct
{
   MutableStringView *stringFirst;
   MutableStringView *stringLast;
} StringBucket;
typedef struct
{
   StringBucket buckets[1 << NAME_BUCKET_EXPONENT];
} StringHash;
 
static StringHash* getNameTable()
{
   static StringHash* sHash;
   if (!sHash)
   {
      sHash = calloc(1, sizeof(StringHash));
   }
   return sHash;
}
static size_t hashName(StringView name)
{
   size_t out = 5381;
   const char* c = name;
   while (*c)
   {
      out = (out << 5) + (out << 1) + *c++;
   }
   return out & ((1 << NAME_BUCKET_EXPONENT)-1);
}
 
StringView stringBucketIntern(StringBucket* bucket, StringView view)
{
   int count;
   StringView* first,*last;
 
   int len = strlen(view);
   first = bucket->stringFirst;
   last = bucket->stringLast;
   count = last-first;
 
   while (first != last)
   {
      if (!strcmp(*first, view)) return *first;
      ++first;
   }
 
   //need to grow this bucket.
   bucket->stringFirst = realloc(bucket->stringFirst, sizeof(StringView) * (count+1));
   bucket->stringLast = bucket->stringFirst + (count+1);
   bucket->stringFirst[count] = malloc(len+1);
   memcpy(bucket->stringFirst[count], view, len+1);
   return bucket->stringFirst[count];

}
 
StringView stringIntern(StringView view)
{
   size_t idx = hashName(view);
   StringHash* table = getNameTable();
   StringBucket* bucket = &table->buckets[idx];
   return stringBucketIntern(bucket, view);
}