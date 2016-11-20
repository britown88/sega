#include "Preprocessor.h"


#ifndef vec
#define vec(TYPE) CONCAT(vec_, TYPE)
#define vecCreate(TYPE) CONCAT(vecCreate_, TYPE)
#define vecDestroy(TYPE) CONCAT(vecDestroy_, TYPE)
#define vecResize(TYPE) CONCAT(vecResize_, TYPE)
#define vecPushBack(TYPE) CONCAT(vecPushBack_, TYPE)
#define vecPushArray(TYPE) CONCAT(vecPushArray_, TYPE)
#define vecInitArray(TYPE) CONCAT(vecInitArray_, TYPE)
#define vecPopBack(TYPE) CONCAT(vecPopBack_, TYPE)
#define vecInsert(TYPE) CONCAT(vecInsert_, TYPE)
#define vecAt(TYPE) CONCAT(vecAt_, TYPE)
#define vecIsEmpty(TYPE) CONCAT(vecIsEmpty_, TYPE)
#define vecSize(TYPE) CONCAT(vecSize_, TYPE)
#define vecClear(TYPE) CONCAT(vecClear_, TYPE)
#define vecBegin(TYPE) CONCAT(vecBegin_, TYPE)
#define vecEnd(TYPE) CONCAT(vecEnd_, TYPE)
#define vecBack(TYPE) CONCAT(vecBack_, TYPE)
#define vecReverse(TYPE) CONCAT(vecReverse_, TYPE)
#define vecIndexOf(TYPE) CONCAT(vecIndexOf_, TYPE)
#define vecRemove(TYPE) CONCAT(vecDelete_, TYPE)
#define vecRemoveAt(TYPE) CONCAT(vecDeleteAt_, TYPE)
#define vecSort(TYPE) CONCAT(vecSort_, TYPE)

#define vecPushStackArray(TYPE, list, ...) vecPushArray(TYPE)(list, (TYPE[])__VA_ARGS__, sizeof((TYPE[])__VA_ARGS__) / sizeof(TYPE))

#define vecForEach(TYPE, varName, list, ...) {\
   vec(TYPE) *CONCAT(__temp_, vec(TYPE)) = list; /*cache the list before use*/ \
   if(!vecIsEmpty(TYPE)(CONCAT(__temp_, vec(TYPE)))){\
      TYPE *varName = vecBegin(TYPE)(CONCAT(__temp_, vec(TYPE))); \
      TYPE *CONCAT(__end_, TYPE) = vecEnd(TYPE)(CONCAT(__temp_, vec(TYPE))); \
      \
      for(; varName != CONCAT(__end_, TYPE); ++varName){ \
         __VA_ARGS__ \
      }\
   } \
}


#endif
