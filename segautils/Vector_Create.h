#include "Preprocessor.h"
#include "BitTwiddling.h"
#include "Defs.h"
#include "segashared\CheckedMemory.h"
#define VEC_NAME CONCAT(vec_, T)

#ifndef vec
#define vec(TYPE) CONCAT(vec_, TYPE)
#define vecCreate(TYPE) CONCAT(vecCreate_, TYPE)
#define vecDestroy(TYPE) CONCAT(vecDestroy_, TYPE)
#define vecResize(TYPE) CONCAT(vecResize_, TYPE)
#define vecPushBack(TYPE) CONCAT(vecPushBack_, TYPE)
#define vecAt(TYPE) CONCAT(vecAt_, TYPE)
#define vecIsEmpty(TYPE) CONCAT(vecIsEmpty_, TYPE)
#define vecClear(TYPE) CONCAT(vecClear_, TYPE)
#endif

typedef struct {
   void (*destroy)(T*);

   T *data;

   //number of items in the table
   size_t count;

   size_t alloc;
} VEC_NAME;

static VEC_NAME *vecCreate(T)(void(*destroy)(T*)){
   VEC_NAME *out = checkedCalloc(1, sizeof(VEC_NAME));
   out->destroy = destroy;
   return out;
}
static void vecDestroy(T)(VEC_NAME *self){
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
static void vecResize(T)(VEC_NAME *self, size_t size, T *initialValue){
   if (size != self->count){//size has changed
      if (size > self->count){//size has increased
         size_t i;

         if (!self->data) {//list is empty
            //init
            self->alloc = 1 << (BSR32(size) + 1);
            self->data = checkedCalloc(1, sizeof(T) * self->alloc);

         }
         else if (size >= self->alloc) {
            //list over 75% full
            T *newList;
            self->alloc = 1 << (BSR32(size) + 1);

            newList = checkedCalloc(1, sizeof(T) * self->alloc);
            memcpy(newList, self->data, sizeof(T) * self->count);
            checkedFree(self->data);
            self->data = newList;
         }

         if (initialValue){
            for (i = self->count; i < size; ++i){
               if (self->destroy && self->data + i){
                  self->destroy(self->data + i);
               }
               memcpy(self->data + i, initialValue, sizeof(T));
            }
         }
      }
      else if(self->destroy){
         //size has decreased
         size_t i;
         for (i = self->count; i > size; --i){
            if (self->data + i){
               self->destroy(self->data + i);
            }
         }
      }

      self->count = size;

   }   
}
static void vecPushBack(T)(VEC_NAME *self, T *data){
   vecResize(T)(self, self->count + 1, data);
}
static T *vecAt(T)(VEC_NAME *self, size_t index){
   return self->data + index;
}
static int vecIsEmpty(T)(VEC_NAME *self){
   return self->count == 0;
}
static void vecClear(T)(VEC_NAME *self){
   vecResize(T)(self, 0, 0);
}

#undef VEC_NAME
#undef T