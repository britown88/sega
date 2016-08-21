#pragma once

#include <stdint.h>

typedef int bool;
#define false 0
#define true 1

#define SIGN(expr) ((expr==0)?0:((expr>0)?1:-1))

typedef unsigned char byte;

#define INF ((size_t)-1)
#define INFF ((float)INF)

#define SQRT2 (1.414213f)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define EMPTY_STRUCT int UNUSED

#define SEGASSERT(...) if(!(__VA_ARGS__)){ int a = *(int*)0; }
/*
typedef struct {
   EMPTY_STRUCT;
} foo;
*/

