#pragma once

#include <malloc.h>
#include "extern_c.h"

SEXTERN_C

void* checkedMallocImpl(size_t sz, char* file, size_t line);
void* checkedCallocImpl(size_t count, size_t sz, char* file, size_t line);
void checkedFreeImpl(void* mem);
void printMemoryLeaks();

END_SEXTERN_C
 
#ifdef _DEBUG
#define checkedMalloc(sz) checkedMallocImpl(sz, __FILE__, __LINE__)
#define checkedCalloc(count, sz) checkedCallocImpl(count, sz, __FILE__, __LINE__)
#define checkedFree(sz) checkedFreeImpl(sz)
#else
#define checkedMalloc(sz) malloc(sz)
#define checkedCalloc(count, sz) calloc(count, sz)
#define checkedFree(sz) free(sz)
#endif


 
