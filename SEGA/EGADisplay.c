#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "bt-utils\CheckedMemory.h"
#include "segalib\EGA.h"
#include "EGADisplay.h"
#include "EGAPalette.h"
#include "bt-utils\Defs.h"
#include "VBO.h"
#include "Vector.h"
#include "Renderer.h"
#include "Shader.h"

#include "bt-utils\Strings.h"

#include <malloc.h>
#include <string.h>

typedef struct EGADisplay_t {
   VBO *vbo;
   Shader *shader;
   Matrix model;

   EGAPalette *currentPalette;

   EGAPalette **internPalettes;
   int internPaletteCount;
   int internPaletteAlloc;
};

EGADisplay *egaDisplayCreate() {
   EGADisplay *r = checkedCalloc(1, sizeof(EGADisplay));
   
   Float2 *posData;
   int x, y;

   //alloc the internal palette storage
   r->internPaletteAlloc = 1;
   r->internPalettes = checkedCalloc(1, sizeof(EGAPalette *));

   //build the vbo
   r->vbo = vboCreate();

   vboAddAttribute(r->vbo, saPosition, false);
   vboAddAttribute(r->vbo, saColor, true);

   vboBuild(r->vbo, EGA_RES_HEIGHT * EGA_RES_WIDTH); 

   //now fill in the position vertices
   posData = vboGetAttributeChunk(r->vbo, saPosition);

   for(y = 0; y < EGA_RES_HEIGHT; ++y) {
      for(x = 0; x < EGA_RES_WIDTH; ++x) {
         posData[y * EGA_RES_WIDTH + x] = float2Create((float)x, (float)y);
      }
   }

   //init the shader
   r->shader = shaderCreate("assets/shaders/sega.vert", "assets/shaders/sega.frag");

   matrixIdentity(&r->model);
   matrixTranslate(&r->model, 0.0f, 1.0f); //points are all off by one for some reason

   return r;
}

void egaDisplayDestroy(EGADisplay *self) {
   int i;
   for(i = 0; i < self->internPaletteCount; ++i)
      egaPaletteDestroy(self->internPalettes[i]);

   checkedFree(self->internPalettes);

   vboDestroy(self->vbo);
   shaderDestroy(self->shader);
   checkedFree(self);
}

EGAPalette *egaDisplayInternPalette(EGADisplay *self, byte *p) {
   EGAPalette *r = egaPaletteCreate(p);

   if(self->internPaletteCount == self->internPaletteAlloc) {
      //new alloc
      EGAPalette **newArr;

      self->internPaletteAlloc *= 2;
      newArr = checkedCalloc(1, sizeof(EGAPalette*) * self->internPaletteAlloc);

      memcpy(newArr, self->internPalettes, sizeof(EGAPalette*) * self->internPaletteCount);
      checkedFree(self->internPalettes);
      self->internPalettes = newArr;
   }

   self->internPalettes[self->internPaletteCount++] = r;

   return r;
}

void egaDisplaySetPalette(EGADisplay *self, EGAPalette *p) {
   self->currentPalette = p;
}

void egaDisplayRender(EGADisplay *self, Renderer *r) {
   static Rectf imgBounds = {0.0f, 0.0f, (float)EGA_RES_WIDTH, (float)EGA_RES_HEIGHT};

   rendererPushCamera(r, imgBounds);

      shaderSetActive(self->shader);
      shaderSet4fv(self->shader, "u_viewMatrix", rendererGetViewMatrix(r));
      shaderSet4fv(self->shader, "u_modelMatrix", &self->model);

      if(self->currentPalette) {
         shaderSet1i(self->shader, "u_palette", 0);
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_1D, egaPaletteGetHandle(self->currentPalette));
      }

      vboMakeCurrent(self->vbo);
      glDrawArrays(GL_POINTS, 0, vboGetVertexCount(self->vbo));

   rendererPopCamera(r);
}

void egaDisplayRenderFrame(EGADisplay *self, Frame *fb) {
   int x, y, i, j;
   float *cData = vboGetAttributeChunk(self->vbo, saColor);

   for(y = 0; y < EGA_RES_HEIGHT; ++y){
      for(x = 0; x < EGA_RES_WIDTH/8;++x) {
         int color = 0;
         byte colors[8] = {0};

         for(i = 0; i < EGA_PLANES; ++i) {
            color += (fb->planes[i].lines[y].pixels[x]) << (i*8);
         }

         for(i = 0; i < 8; ++i) {
            int shiftedColor = color >> i;

            for(j = 0; j < EGA_PLANES; ++j) {
               int mask = 1<<(j*8);				
               colors[i] += !!(shiftedColor&mask) << j;
            }				
         }

         for(i = 0; i < 8; ++i) {
            *cData++ = colors[i] / 15.0f;
         }
      }
   }

   vboPush(self->vbo);  

}