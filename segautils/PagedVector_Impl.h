#include "Preprocessor.h"
#include "BitTwiddling.h"
#include "Defs.h"
#include "segashared\CheckedMemory.h"
#include <string.h>

#define T PVectorTPart
#define VEC_NAME CONCAT(pvec_, T)

#include "PagedVector_Functions.h"

typedef void* VoidPtr;
typedef struct vec_VoidPtr_t vec_VoidPtr;

struct CONCAT(VEC_NAME, _t){
   void(*destroy)(T*);

   //number of items in the table
   size_t count;
   size_t pageSize;

   vec(VoidPtr) *intern;
};

static pvecInternDestroy(T)(VoidPtr *page){
   checkedFree(*page);
}

VEC_NAME *pvecCreate(T)(size_t pageSize, void(*destroy)(T*)){
   VEC_NAME *out = checkedCalloc(1, sizeof(VEC_NAME));

   out->destroy = destroy;
   out->intern = vecCreate(VoidPtr)(&pvecInternDestroy(T));
   return out;
}
void pvecDestroy(T)(VEC_NAME *self){
   //destroy buckets here
   if (self->intern) {
      if (self->destroy){
         size_t pCount = vecSize(VoidPtr)(self->intern);
         size_t p = 0, i = 0, j = 0;
         for (p = 0; p < pCount; ++p){
            for (j = 0; j < self->pageSize && i < self->count; ++i, ++j){ 
               T *arr = *vecAt(VoidPtr)(self->intern, p);
               self->destroy( arr + i);
            }
         }
      }
      vecDestroy(VoidPtr)(self->intern);
   }
   checkedFree(self);
}
void pvecResize(T)(VEC_NAME *self, size_t size, T *initialValue){
   //if (size != self->count){//size has changed
   //   if (size > self->count){//size has increased
   //      size_t i;

   //      if (!self->data) {//list is empty
   //         //init
   //         self->alloc = 1 << (BSR32(size) + 1);
   //         self->data = checkedCalloc(1, sizeof(T) * self->alloc);

   //      }
   //      else if (size >= self->alloc) {
   //         T *newList;
   //         self->alloc = 1 << (BSR32(size) + 1);

   //         newList = checkedCalloc(1, sizeof(T) * self->alloc);
   //         memcpy(newList, self->data, sizeof(T) * self->count);
   //         checkedFree(self->data);
   //         self->data = newList;
   //      }

   //      if (initialValue){
   //         for (i = self->count; i < size; ++i){
   //            memcpy(self->data + i, initialValue, sizeof(T));
   //         }
   //      }
   //   }
   //   else if (self->destroy){
   //      //size has decreased
   //      size_t i;
   //      for (i = self->count; i > size; --i){
   //         if (self->data + (i - 1)){
   //            self->destroy(self->data + (i - 1));
   //         }
   //      }
   //   }

   //   self->count = size;

   //}
}
void pvecPushBack(T)(VEC_NAME *self, T *data){
   pvecResize(T)(self, self->count + 1, data);
}
void pvecPopBack(T)(VEC_NAME *self){
   if (self->count > 0){
      pvecResize(T)(self, self->count - 1, NULL);
   }
}
T *pvecAt(T)(VEC_NAME *self, size_t index){
   T* arr;
   if (index >= self->count){
      return NULL;
   }

   arr = *vecAt(VoidPtr)(self->intern, index / self->pageSize);

   return arr + (index % self->pageSize);
}
int pvecIsEmpty(T)(VEC_NAME *self){
   return self->count == 0;
}
size_t pvecSize(T)(VEC_NAME *self){
   return self->count;
}
void pvecClear(T)(VEC_NAME *self){
   pvecResize(T)(self, 0, NULL);
}
T *pvecBegin(T)(VEC_NAME *self){
   return !pvecIsEmpty(T)(self) ? pvecAt(T)(self, 0) : NULL;
}
T *pvecEnd(T)(VEC_NAME *self){
   return !pvecIsEmpty(T)(self) ? pvecAt(T)(self, pvecSize(T)(self)) : NULL;
}


#undef VEC_NAME
#undef PVectorTPart
#undef T
