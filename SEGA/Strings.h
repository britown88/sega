#pragma once

#include "Defs.h"

typedef const char* StringView;
typedef char* MutableStringView;
StringView stringIntern(StringView view);