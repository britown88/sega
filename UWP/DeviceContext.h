#pragma once

#include "segautils/extern_c.h"
#include "App.h"

SEXTERN_C
typedef struct IDeviceContext_t IDeviceContext;

IDeviceContext *createUWPContext(UWP::App ^app);
END_SEXTERN_C