#pragma once

#include "segautils/extern_c.h"

SEXTERN_C
typedef struct IRenderer_t IRenderer;
typedef struct IDeviceContext_t IDeviceContext;

IRenderer *createUWPRenderer(IDeviceContext *dc);
END_SEXTERN_C