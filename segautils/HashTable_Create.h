#include "Preprocessor.h"
#include "segashared\CheckedMemory.h"
#include <string.h>

#define T HashTableT

#define HT_NAME CONCAT(ht_, T)
#define HT_BUCKET CONCAT(htbucket_, T)

#include "HashTable_Functions.h"

typedef struct CONCAT(HT_BUCKET, _t) {
   struct CONCAT(HT_BUCKET, _t) *next;
   T data;
} HT_BUCKET;

typedef struct {
   int (*compare)(T*, T*);
   size_t (*hash)(T*);
   void (*destroy)(T*);

   HT_BUCKET **buckets;

   //number of items in the table
   size_t count;

   //has 2^power buckets allocated
   size_t power;
} HT_NAME;

static HT_NAME *htCreate(T)(int (*compare)(T*, T*), size_t (*hash)(T*), void (*destroy)(T*)){

#ifndef UNCHECKED
   HT_NAME *r = checkedCalloc(1, sizeof(HT_NAME));
#else
   HT_NAME *r = calloc(1, sizeof(HT_NAME));
#endif

   r->compare = compare;
   r->hash = hash;
   r->destroy = destroy;
   return r;
}

static void htDestroy(T)(HT_NAME *self){
   //destroy buckets here
   if(self->buckets) {
      HT_BUCKET **bucket = self->buckets;
      HT_BUCKET **end = bucket + (1 << self->power);

      for(;bucket != end; ++bucket){
         HT_BUCKET *iter = *bucket;
         while(iter){
            HT_BUCKET *next = iter->next;
            if(self->destroy){
               self->destroy(&iter->data);
            }
#ifndef UNCHECKED
            checkedFree(iter);  
#else
            free(iter);
#endif
                   
            iter = next;
         }
      }
#ifndef UNCHECKED
      checkedFree(self->buckets);
#else
      free(self->buckets);
#endif
      
   }
#ifndef UNCHECKED
   checkedFree(self);
#else
   free(self);
#endif
   
}

static void htClear(T)(HT_NAME *self) {
   //destroy buckets here
   if (self->buckets) {
      HT_BUCKET **bucket = self->buckets;
      HT_BUCKET **end = bucket + (1 << self->power);

      for (;bucket != end; ++bucket) {
         HT_BUCKET *iter = *bucket;
         while (iter) {
            HT_BUCKET *next = iter->next;
            if (self->destroy) {
               self->destroy(&iter->data);
            }
#ifndef UNCHECKED
            checkedFree(iter);
#else
            free(iter);
#endif

            iter = next;
         }
      }
   }
   self->count = 0;




}


static void htErase(T)(HT_NAME *self, T *item){
   if(!self->buckets) {
      return;
   }
   else {
      int hashed = self->hash(item) & ((1 << self->power) - 1);

      HT_BUCKET *iter = self->buckets[hashed];
      HT_BUCKET *prev = 0;
      while(iter){
         if(self->compare(item, &iter->data)){
            //FOUND IT GUISE
            if(prev){
               prev->next = iter->next;
            }
            else {
               self->buckets[hashed] = iter->next;
            }

            if(self->destroy){
               self->destroy(&iter->data);
            }
#ifndef UNCHECKED
            checkedFree(iter);
#else
            free(iter);
#endif       
            --self->count;
            return;
         }

         prev = iter;
         iter = iter->next;         
      }
   }
}

static T *htFind(T)(HT_NAME *self, T *item){
   if(!self->buckets) {
      return 0;
   }
   else {
      int hashed = self->hash(item) & ((1 << self->power) - 1);

      HT_BUCKET *iter = self->buckets[hashed];
      while(iter){
         if(self->compare(item, &iter->data)){
            //FOUND IT GUISE
            return &iter->data;
         }
         iter = iter->next;         
      }

      return 0;
   }
}

static void _htGrow(T)(HT_NAME *self){
   HT_BUCKET **buckets = self->buckets;
   HT_BUCKET **lastBucket = buckets + (1 << self->power);
   HT_BUCKET **firstBucket = buckets;

   ++self->power;
#ifndef UNCHECKED
   self->buckets = checkedCalloc(1, sizeof(HT_BUCKET*) * (1 << self->power));
#else
   self->buckets = calloc(1, sizeof(HT_BUCKET*) * (1 << self->power));
#endif   

   for(;buckets != lastBucket; ++buckets){
      HT_BUCKET *iter = *buckets;
      while(iter){
         HT_BUCKET *next = iter->next;
         int hashed = self->hash(&iter->data) & ((1 << self->power) - 1);

         //Take the beginning of the list and make it nmext of this iter
         iter->next = self->buckets[hashed];

         //now we can put that iter at the beginning of the list and we gucci
         self->buckets[hashed] = iter;

         iter = next;
      }
   }
#ifndef UNCHECKED
   checkedFree(firstBucket);
#else
   free(firstBucket);
#endif
   
}


static void htInsert(T)(HT_NAME *self, T *item){
   int hashed;
   HT_BUCKET **bucket;

   //Grow the hashtable
   if(!self->buckets) {
      //init
      self->power = 5;
#ifndef UNCHECKED
      self->buckets = checkedCalloc(1, sizeof(HT_BUCKET*) * (1 << self->power));
#else
      self->buckets = calloc(1, sizeof(HT_BUCKET*) * (1 << self->power));
#endif

   }
   else if(self->count > (size_t)((1 << (self->power - 1)) + (1 << (self->power - 2)))) {
      //table over 75% full
      _htGrow(T)(self);
   }

   hashed = self->hash(item) & ((1 << self->power) - 1);
   bucket = self->buckets + hashed;

   if(!*bucket){
      //theres no bucket here
#ifndef UNCHECKED
      HT_BUCKET *newBucket = checkedCalloc(1, sizeof(HT_BUCKET));
#else
      HT_BUCKET *newBucket = calloc(1, sizeof(HT_BUCKET));
#endif      
      memcpy(&newBucket->data, item, sizeof(T));

      *bucket = newBucket;
      ++self->count;
   }
   else {
      //add to existing bucket
      HT_BUCKET *iter = *bucket;

      while(iter){
         if(self->compare(item, &iter->data)){
            //data already exists
            if(self->destroy){
               self->destroy(&iter->data);
            }
            memcpy(&iter->data, item, sizeof(T));
            return;
         }

         if(!iter->next){
            break;
         }
         else {
            iter = iter->next;
         }
      }

      {
         //doesnt exist in hash table
#ifndef UNCHECKED
         HT_BUCKET *newBucket = checkedCalloc(1, sizeof(HT_BUCKET));
#else
         HT_BUCKET *newBucket = calloc(1, sizeof(HT_BUCKET));
#endif         
         memcpy(&newBucket->data, item, sizeof(T));
         iter->next = newBucket;
         ++self->count;
      }
   }         
}

#undef HT_BUCKET
#undef HT_NAME
#undef HashTableT
#undef T