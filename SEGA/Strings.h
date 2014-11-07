#pragma once

#include "Defs.h"
#include "DLLBullshit.h"

typedef const char* StringView;
typedef char* MutableStringView;
DLL_PUBLIC StringView stringIntern(StringView view);