#pragma once

#include "segautils/extern_c.h"

SEXTERN_C
typedef struct IDeviceContext_t IDeviceContext;

IDeviceContext *createUWPContext();
END_SEXTERN_C