#include "Preprocessor.h"
#include <string.h>

#define T PVectorTPart
#define VEC_NAME CONCAT(pvec_, T)

typedef struct CONCAT(VEC_NAME, _t) VEC_NAME;

#include "PagedVector_Functions.h"

VEC_NAME *pvecCreate(T)(size_t pageSize, void(*destroy)(T*));
void pvecDestroy(T)(VEC_NAME *self);
void pvecResize(T)(VEC_NAME *self, size_t size, T *initialValue);
void pvecPushBack(T)(VEC_NAME *self, T *data);
void pvecPopBack(T)(VEC_NAME *self);
T *pvecAt(T)(VEC_NAME *self, size_t index);
int pvecIsEmpty(T)(VEC_NAME *self);
size_t pvecSize(T)(VEC_NAME *self);
void pvecClear(T)(VEC_NAME *self);
T *pvecBegin(T)(VEC_NAME *self);
T *pvecEnd(T)(VEC_NAME *self);

#undef VEC_NAME
#undef PVectorTPart
#undef T