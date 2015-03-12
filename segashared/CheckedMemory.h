#pragma once

#include <malloc.h>
#include "segautils\extern_c.h"
#include "segautils\DLLBullshit.h"

SEXTERN_C

DLL_PUBLIC void* checkedMallocImpl(size_t sz, char* file, size_t line);
DLL_PUBLIC void* checkedCallocImpl(size_t count, size_t sz, char* file, size_t line);
DLL_PUBLIC void* uncheckedMallocImpl(size_t sz, char* file, size_t line);
DLL_PUBLIC void* uncheckedCallocImpl(size_t count, size_t sz, char* file, size_t line);
DLL_PUBLIC void checkedFreeImpl(void* mem);
DLL_PUBLIC void printMemoryLeaks();

END_SEXTERN_C
 
#ifdef _DEBUG
#define checkedMalloc(sz) checkedMallocImpl(sz, __FILE__, __LINE__)
#define checkedCalloc(count, sz) checkedCallocImpl(count, sz, __FILE__, __LINE__)
#define checkedFree(sz) checkedFreeImpl(sz)
#define uncheckedMalloc(sz) uncheckedMallocImpl(sz, __FILE__, __LINE__)
#define uncheckedCalloc(count, sz) uncheckedCallocImpl(count, sz, __FILE__, __LINE__)
#else
#define checkedMalloc(sz) malloc(sz)
#define checkedCalloc(count, sz) calloc(count, sz)
#define checkedFree(sz) free(sz)
#define uncheckedMalloc(sz) malloc(sz)
#define uncheckedCalloc(count, sz) calloc(count, sz)
#endif


 
