#include "Preprocessor.h"
#include "BitTwiddling.h"
#include "Defs.h"
#include "segashared\CheckedMemory.h"
#include <string.h>

#define T VectorTPart
#define VEC_NAME CONCAT(vec_, T)

#include "Vector_Functions.h"

struct CONCAT(VEC_NAME, _t){
   T *data;

   void(*destroy)(T*);

   //number of items in the table
   size_t count;

   size_t alloc;
};

VEC_NAME *vecCreate(T)(void(*destroy)(T*)){
   VEC_NAME *out = checkedCalloc(1, sizeof(VEC_NAME));
   out->destroy = destroy;
   return out;
}
void vecDestroy(T)(VEC_NAME *self){
   //destroy buckets here
   if (self->data) {
      if (self->destroy){
         size_t i = 0;
         for (i = 0; i < self->count; ++i){
            self->destroy(&self->data[i]);
         }
      }
      checkedFree(self->data);
   }
   checkedFree(self);
}
void vecResize(T)(VEC_NAME *self, size_t size, T *initialValue){
   if (size != self->count){//size has changed
      if (size > self->count){//size has increased
         size_t i;

         if (!self->data) {//list is empty
            //init
            self->alloc = 1 << (BSR32(size) + 1);
            self->data = checkedCalloc(1, sizeof(T) * self->alloc);

         }
         else if (size >= self->alloc) {
            T *newList;
            self->alloc = 1 << (BSR32(size) + 1);

            newList = checkedCalloc(1, sizeof(T) * self->alloc);
            memcpy(newList, self->data, sizeof(T) * self->count);
            checkedFree(self->data);
            self->data = newList;
         }

         if (initialValue){
            for (i = self->count; i < size; ++i){
               memcpy(self->data + i, initialValue, sizeof(T));
            }
         }
      }
      else if (self->destroy){
         //size has decreased
         size_t i;
         for (i = self->count; i > size; --i){
            if (self->data + (i - 1)){
               self->destroy(self->data + (i - 1));
            }
         }
      }

      self->count = size;

   }
}
void vecPushArray(T)(VEC_NAME *self, T*arr, size_t count){
   vecResize(T)(self, self->count + count, NULL);
   memcpy(self->data + self->count - count, arr, count * sizeof(T));
}
VEC_NAME *vecInitArray(T)(void(*destroy)(T*), T*arr, size_t count) {
   VEC_NAME *out = vecCreate(T)(destroy);
   vecPushArray(T)(out, arr, count);
   return out;
}

void vecPushBack(T)(VEC_NAME *self, T *data){
   vecResize(T)(self, self->count + 1, data);
}
void vecPopBack(T)(VEC_NAME *self){
   if (self->count > 0){
      vecResize(T)(self, self->count - 1, NULL);
   }
}
void vecInsert(T)(VEC_NAME *self, size_t pos, T*data){
   if (pos <= self->count){
      vecResize(T)(self, self->count + 1, NULL);
      memmove(self->data + pos + 1,
         self->data + pos,
         sizeof(T) * (self->count - 1 - pos));
      memcpy(self->data + pos, data, sizeof(T));
   }
}
T *vecAt(T)(VEC_NAME *self, size_t index){
   return self->data + index;
}
int vecIsEmpty(T)(VEC_NAME *self){
   return self->count == 0;
}
size_t vecSize(T)(VEC_NAME *self){
   return self->count;
}
void vecClear(T)(VEC_NAME *self){
   vecResize(T)(self, 0, NULL);
}
T *vecBegin(T)(VEC_NAME *self){
   return !vecIsEmpty(T)(self) ? vecAt(T)(self, 0) : NULL;
}
T *vecEnd(T)(VEC_NAME *self){
   return !vecIsEmpty(T)(self) ? vecAt(T)(self, vecSize(T)(self)) : NULL;
}
T *vecBack(T)(VEC_NAME *self){
   return !vecIsEmpty(T)(self) ? vecAt(T)(self, vecSize(T)(self) - 1) : NULL;
}
void vecReverse(T)(VEC_NAME *self){
   size_t count = vecSize(T)(self);
   size_t i;
   for (i = 0; i < count / 2; ++i){
      T temp = self->data[i];
      self->data[i] = self->data[count - i - 1];
      self->data[count - i - 1] = temp;
   }
}

size_t vecIndexOf(T)(VEC_NAME *self, T*item){
   size_t i = 0, out = INF;
   vecForEach(T, iter, self, {
      if (!memcmp(iter, item, sizeof(T))){
         out = i;
         break;
      }
      ++i;
   });

   return out;
}

void vecRemove(T)(VEC_NAME *self, T*item){
   size_t i = vecIndexOf(T)(self, item);
   if (i < INF){
      vecRemoveAt(T)(self, i);
   }
}

void vecRemoveAt(T)(VEC_NAME *self, size_t index){
   if (index < self->count){
      if (self->destroy){
         self->destroy(self->data + index);
      }

      memcpy(self->data + index,
         self->data + index + 1,
         sizeof(T) * (self->count-- - 1 - index));      
   }
}

#define vecSort_PARTITION(TYPE) CONCAT(vecSort_PARTITION_, TYPE)
#define vecSort_QUICKSORT(TYPE) CONCAT(vecSort_QUICKSORT_, TYPE)

static size_t vecSort_PARTITION(T)(VEC_NAME *self, bool(*lessThan)(T*, T*), size_t lo, size_t hi) {
   T *pivot = vecAt(T)(self, hi);
   size_t i = lo;
   size_t j = 0;

   for (j = lo; j < hi; ++j) {
      T *atJ = vecAt(T)(self, j);
      if (lessThan(atJ, pivot)) {
         T t = self->data[i];
         self->data[i] = self->data[j];
         self->data[j] = t;

         ++i;
      }
   }

   //swap
   {
      T t = self->data[i];
      self->data[i] = self->data[hi];
      self->data[hi] = t;
   }

   return i;
}

static void vecSort_QUICKSORT(T)(VEC_NAME *self, bool(*lessThan)(T*, T*), size_t lo, size_t hi) {
   T *l = vecAt(T)(self, lo);
   T *h = vecAt(T)(self, hi);

   if (lo < hi) {
      size_t p = vecSort_PARTITION(T)(self, lessThan, lo, hi);
      if (p > 0) {
         vecSort_QUICKSORT(T)(self, lessThan, lo, p - 1);
      }
      
      vecSort_QUICKSORT(T)(self, lessThan, p + 1, hi);
   }
}

void vecSort(T)(VEC_NAME *self, bool(*lessThan)(T*, T*)) {
   vecSort_QUICKSORT(T)(self, lessThan, 0, self->count - 1);
}

#undef vecSort_PARTITION
#undef vecSort_QUICKSORT

#undef VEC_NAME
#undef VectorTPart
#undef T
