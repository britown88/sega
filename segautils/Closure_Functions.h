#include "Preprocessor.h"

#ifndef CLOSURE_TYPES_DEFINED
#define CLOSURE_TYPES_DEFINED
typedef void* ClosureData;

#define closureInit(ClosureName) CONCAT(ClosureName, Init)
#define closureDestroy(ClosureName) CONCAT(ClosureName, Destroy)
#define closureCall(ClosurePtr, ...) (ClosurePtr)->func((ClosurePtr)->data, __VA_ARGS__)
#define closureDestroy(ClosureName) CONCAT(ClosureName, Destroy)
#define closureIsNull(ClosureName) CONCAT(ClosureName, IsNull)
#endif