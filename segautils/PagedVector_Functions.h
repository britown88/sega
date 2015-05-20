#include "Preprocessor.h"

#ifndef pvec
#define pvec(TYPE) CONCAT(pvec_, TYPE)
#define pvecCreate(TYPE) CONCAT(pvecCreate_, TYPE)
#define pvecInternDestroy(TYPE) CONCAT(pvecInternDestroy_, TYPE)
#define pvecDestroy(TYPE) CONCAT(pvecDestroy_, TYPE)
#define pvecResize(TYPE) CONCAT(pvecResize_, TYPE)
#define pvecPushBack(TYPE) CONCAT(pvecPushBack_, TYPE)
#define pvecPopBack(TYPE) CONCAT(pvecPopBack_, TYPE)
#define pvecAt(TYPE) CONCAT(pvecAt_, TYPE)
#define pvecIsEmpty(TYPE) CONCAT(pvecIsEmpty_, TYPE)
#define pvecSize(TYPE) CONCAT(pvecSize_, TYPE)
#define pvecClear(TYPE) CONCAT(pvecClear_, TYPE)
#define pvecBegin(TYPE) CONCAT(pvecBegin_, TYPE)
#define pvecEnd(TYPE) CONCAT(pvecEnd_, TYPE)

#define pvecPushStackArray(TYPE, list, ...) pvecPushArray(TYPE)(list, (TYPE[])__VA_ARGS__, sizeof((TYPE[])__VA_ARGS__) / sizeof(TYPE))

#define pvecForEach(TYPE, varName, list, ...) {\
   pvec(TYPE) *CONCAT(__temp_, pvec(TYPE)) = list; /*cache the list before use*/ \
   if(!pvecIsEmpty(TYPE)(CONCAT(__temp_, pvec(TYPE)))){\
      TYPE *varName = pvecBegin(TYPE)(CONCAT(__temp_, pvec(TYPE))); \
      TYPE *CONCAT(__end_, TYPE) = pvecEnd(TYPE)(CONCAT(__temp_, pvec(TYPE))); \
      \
      for(; varName != CONCAT(__end_, TYPE); ++varName){ \
         __VA_ARGS__ \
            }\
      } \
}


#endif
