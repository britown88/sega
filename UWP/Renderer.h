#pragma once

#include "segautils/extern_c.h"
#include "App.h"

SEXTERN_C
typedef struct IRenderer_t IRenderer;
typedef struct IDeviceContext_t IDeviceContext;

IRenderer *createUWPRenderer(IDeviceContext *dc, UWP::UWPMain *main);
END_SEXTERN_C