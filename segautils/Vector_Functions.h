#include "Preprocessor.h"

#ifndef vec
#define vec(TYPE) CONCAT(vec_, TYPE)
#define vecCreate(TYPE) CONCAT(vecCreate_, TYPE)
#define vecDestroy(TYPE) CONCAT(vecDestroy_, TYPE)
#define vecResize(TYPE) CONCAT(vecResize_, TYPE)
#define vecPushBack(TYPE) CONCAT(vecPushBack_, TYPE)
#define vecPopBack(TYPE) CONCAT(vecPopBack_, TYPE)
#define vecAt(TYPE) CONCAT(vecAt_, TYPE)
#define vecIsEmpty(TYPE) CONCAT(vecIsEmpty_, TYPE)
#define vecSize(TYPE) CONCAT(vecSize_, TYPE)
#define vecClear(TYPE) CONCAT(vecClear_, TYPE)
#define vecBegin(TYPE) CONCAT(vecBegin_, TYPE)
#define vecEnd(TYPE) CONCAT(vecEnd_, TYPE)
#define vecReverse(TYPE) CONCAT(vecReverse_, TYPE)
#define vecIndexOf(TYPE) CONCAT(vecIndexOf_, TYPE)
#define vecRemove(TYPE) CONCAT(vecDelete_, TYPE)

#define vecForEach(TYPE, varName, list, ...) \
if(!vecIsEmpty(TYPE)(list)){\
   TYPE *varName = vecBegin(TYPE)(list); \
   TYPE *CONCAT(__end_, TYPE) = vecEnd(TYPE)(list); \
   for(; varName != CONCAT(__end_, TYPE); ++varName) __VA_ARGS__ \
}

#endif
