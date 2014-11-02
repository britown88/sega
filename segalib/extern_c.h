#pragma once

#ifdef __cplusplus
#define SEXTERN_C extern "C" {
#define END_SEXTERN_C };
#else
#define SEXTERN_C 
#define END_SEXTERN_C
#endif

