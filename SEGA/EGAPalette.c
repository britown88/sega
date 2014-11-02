#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "EGAPalette.h"
#include "Color.h"
#include "Strings.h"
#include "segalib\EGA.h"
#include "segalib\CheckedMemory.h"

#include <malloc.h>
#include <string.h>

typedef struct EGAPalette_t {
   byte *imgData;
   unsigned int handle;
};

//16 colors!
EGAPalette *egaPaletteCreate(byte *colors)
{
   EGAPalette *r = checkedMalloc(sizeof(EGAPalette));
   int i;

   r->imgData = checkedCalloc(1, sizeof(Color255) * EGA_PALETTE_COLORS);
   //ACTUALLY FILL IN THE FUCKING ARRAY
   for(i = 0; i < EGA_PALETTE_COLORS; ++i) {
      int color = getEGAColor(colors[i]);
      memcpy(r->imgData + sizeof(Color255) * i, &color, sizeof(Color255));
   }

   glEnable(GL_TEXTURE_1D);
   glGenTextures(1, &r->handle);
   glBindTexture(GL_TEXTURE_1D, r->handle);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, EGA_PALETTE_COLORS, 0,  GL_RGBA, GL_UNSIGNED_BYTE, r->imgData);

   glBindTexture(GL_TEXTURE_1D, 0);

   return r;
}

void egaPaletteDestroy(EGAPalette *self) {
   glDeleteTextures(1, &self->handle);
   checkedFree(self->imgData);
   checkedFree(self);
}

unsigned int egaPaletteGetHandle(EGAPalette *self) {
   return self->handle;   
}