#pragma once

#include <stdint.h>

typedef int bool;
#define false 0
#define true 1

typedef unsigned char byte;

#define INF ((size_t)-1)
#define INFF ((float)INF)

#define SQRT2 (1.414213f)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define EMPTY_STRUCT int UNUSED
/*
typedef struct {
   EMPTY_STRUCT;
} foo;
*/

