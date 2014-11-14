#pragma once

#include "bt-utils\Defs.h"

typedef struct {
   byte r, g, b, a;
} Color255;

static Color255 colorCreate(byte r, byte g, byte b, byte a) {
   Color255 ret = {r, g, b, a};
   return ret;
}