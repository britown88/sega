#pragma once

#include "EGAPalette.h"

typedef struct PaletteTable_t PaletteTable;

PaletteTable *paletteTableCreate();
void paletteTableDestroy(PaletteTable *self);

EGAPalette *paletteTableGetPalette(PaletteTable *self, byte *palette);

