#include "Preprocessor.h"
#include "BitTwiddling.h"
#include "Defs.h"
#include "segashared\CheckedMemory.h"
#include <string.h>

#define T VectorTPart
#define VEC_NAME CONCAT(vec_, T)

#include "Vector_Functions.h"

struct CONCAT(VEC_NAME, _t){
   void(*destroy)(T*);

   T *data;

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

void vecPushBack(T)(VEC_NAME *self, T *data){
   vecResize(T)(self, self->count + 1, data);
}
void vecPopBack(T)(VEC_NAME *self){
   if (self->count > 0){
      vecResize(T)(self, self->count - 1, NULL);
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
   size_t i = 0;
   vecForEach(T, iter, self, {
      if (!memcmp(iter, item, sizeof(T))){
         break;
      }
      ++i;
   });

   return i;
}
void vecRemove(T)(VEC_NAME *self, size_t index){
   if (index < self->count){
      memcpy(self->data + index,
         self->data + index + 1,
         sizeof(T) * (self->count-- - 1 - index));
      
   }
}

#undef VEC_NAME
#undef VectorTPart
#undef T
