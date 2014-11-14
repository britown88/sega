#pragma once

#include "segautils\Defs.h"
#include "segautils\DLLBullshit.h"

typedef const char* StringView;
typedef char* MutableStringView;
DLL_PUBLIC StringView stringIntern(StringView view);