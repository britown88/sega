#pragma once

#include "segautils\Defs.h"
typedef struct EGAPalette_t EGAPalette;

//16 colors!
EGAPalette *egaPaletteCreate(byte *colors);
void egaPaletteDestroy(EGAPalette *self);

unsigned int egaPaletteGetHandle(EGAPalette *self);