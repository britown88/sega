#pragma once

#include <direct.h>
#include <stdio.h>

#include <algorithm>
#include <memory>

#include "EGA.h"

class EGAImage
{
   class Impl;
   std::unique_ptr<Impl> pImpl;
public:
   EGAImage(const char *file);
   ~EGAImage();

   short width();
   short height();

   void renderWithPalette(byte *p, byte offset = 0, byte colorCount = 16, byte totalColors = 16);

   byte *palette();
   Image *toImage();
};
