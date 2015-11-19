#include "Preprocessor.h"
#include <string.h>

#define T VectorTPart
#define VEC_NAME CONCAT(vec_, T)

typedef struct CONCAT(VEC_NAME, _t) VEC_NAME;

#include "Vector_Functions.h"

VEC_NAME *vecCreate(T)(void(*destroy)(T*));
void vecDestroy(T)(VEC_NAME *self);
void vecResize(T)(VEC_NAME *self, size_t size, T *initialValue);
void vecPushBack(T)(VEC_NAME *self, T *data);
void vecPushArray(T)(VEC_NAME *self, T*arr, size_t count);
VEC_NAME *vecInitArray(T)(void(*destroy)(T*), T *arr, size_t count);
void vecPopBack(T)(VEC_NAME *self);
void vecInsert(T)(VEC_NAME *self, size_t pos, T*data);
T *vecAt(T)(VEC_NAME *self, size_t index);
int vecIsEmpty(T)(VEC_NAME *self);
size_t vecSize(T)(VEC_NAME *self);
void vecClear(T)(VEC_NAME *self);
T *vecBegin(T)(VEC_NAME *self);
T *vecEnd(T)(VEC_NAME *self);
T *vecBack(T)(VEC_NAME *self);
void vecReverse(T)(VEC_NAME *self);
size_t vecIndexOf(T)(VEC_NAME *self, T*item);
void vecRemoveAt(T)(VEC_NAME *self, size_t index);
void vecRemove(T)(VEC_NAME *self, T*item);

#undef VEC_NAME
#undef VectorTPart
#undef T