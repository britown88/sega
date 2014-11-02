#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "segalib\CheckedMemory.h"

#include <malloc.h>


typedef struct Renderer_t {
   Rectf vpStack[256];
   Matrix camStack[256];

   int vpCount, camCount;
};

Renderer *rendererCreate() {
   Renderer *r = checkedCalloc(1, sizeof(Renderer));
   return r;
}
void rendererDestroy(Renderer *self) {
   checkedFree(self);
}

void rendererPushViewport(Renderer *self, Rectf bounds) {
   Rectf *vp;
   
   self->vpStack[self->vpCount++] = bounds;
   vp = rendererGetViewport(self);
   glViewport((int)vp->left, (int)vp->top, (int)rectfWidth(vp), (int)rectfHeight(vp));
}
void rendererPopViewport(Renderer *self) {
   if(--self->vpCount < 0) {
      self->vpCount = 0;
   }

   if(self->vpCount) {
      Rectf *vp = rendererGetViewport(self);
      glViewport((int)vp->left, (int)vp->top, (int)rectfWidth(vp), (int)rectfHeight(vp));
   }
}

void rendererPushCamera(Renderer *self, Rectf bounds) {
   Matrix m;
   matrixOrtho(&m, 
         bounds.left, rectfWidth(&bounds ),  
         rectfHeight(&bounds ), bounds.top, 
         1.0f, -1.0f);

   self->camStack[self->camCount++] = m;
}
void rendererPopCamera(Renderer *self) {
   if(--self->camCount < 0) self->camCount = 0;
}

Matrix *rendererGetViewMatrix(Renderer *self) {
   return &self->camStack[self->camCount-1];
}
Rectf *rendererGetViewport(Renderer *self) {
   return &self->vpStack[self->vpCount-1];
}