#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include "segalib\CheckedMemory.h"

#include "FBO.h"
#include "Renderer.h"
#include "Shader.h"
#include "VBO.h"
#include "Vector.h"

#include <malloc.h>

typedef struct FBO_t {
   unsigned int handle, texHandle;
   int width, height;
   Matrix model;
   Shader *shader;
   VBO *vbo;
};

static VBO *_buildVBO() {
   //need to find a cleaner way to do this;
   Float2 p0 = {0.0f, 0.0f};
   Float2 p1 = {1.0f, 0.0f};
   Float2 p2 = {1.0f, 1.0f};
   Float2 p3 = {0.0f, 1.0f};

   VBO *r = vboCreate();
   vboAddAttribute(r, saPosition, false);
   vboAddAttribute(r, saTexCoords, false);
   vboBuild(r, 6);

   vboSetSingleVertex(r, saPosition, 0, &p0);
   vboSetSingleVertex(r, saTexCoords, 0, &p3);

   vboSetSingleVertex(r, saPosition, 1, &p1);
   vboSetSingleVertex(r, saTexCoords, 1, &p2);

   vboSetSingleVertex(r, saPosition, 2, &p3);
   vboSetSingleVertex(r, saTexCoords, 2, &p0);

   vboSetSingleVertex(r, saPosition, 3, &p1);
   vboSetSingleVertex(r, saTexCoords, 3, &p2);

   vboSetSingleVertex(r, saPosition, 4, &p3);
   vboSetSingleVertex(r, saTexCoords, 4, &p0);

   vboSetSingleVertex(r, saPosition, 5, &p2);
   vboSetSingleVertex(r, saTexCoords, 5, &p1);

   return r;
}

FBO *fboCreate(int width, int height) {
   int pixelCount = width * height;
   unsigned char *pixels = (unsigned char*)checkedCalloc(pixelCount * 4, sizeof(unsigned char));
   FBO *r = checkedCalloc(1, sizeof(FBO));

   r->width = width;
   r->height = height;
   r->shader = shaderCreate("assets/shaders/texture.vert", "assets/shaders/texture.frag");

   matrixIdentity(&r->model);
   matrixScale(&r->model, (float)width, (float)height);

   //create the vbo for drawing the texture
   r->vbo = _buildVBO();

   glGenFramebuffers(1, &r->handle);
   glGenTextures(1, &r->texHandle);
   glBindTexture(GL_TEXTURE_2D, r->texHandle);
   glTexImage2D(
      GL_TEXTURE_2D, 0, GL_RGBA8,
      width, height,
      0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

   glBindTexture(GL_TEXTURE_2D, 0);

   checkedFree(pixels);

   //create shader

   return r;
}
void fboDestroy(FBO *self) {
   shaderDestroy(self->shader);
   vboDestroy(self->vbo);
   glDeleteTextures(1, &self->texHandle);
   glDeleteFramebuffers(1, &self->handle);
   checkedFree(self);
}

void fboRender(FBO *self, Renderer *r) {
   Rectf imgBounds = {0.0f, 0.0f, (float)self->width, (float)self->height};

   rendererPushCamera(r, imgBounds);

      shaderSetActive(self->shader);
      shaderSet4fv(self->shader, "u_viewMatrix", rendererGetViewMatrix(r));
      shaderSet4fv(self->shader, "u_modelMatrix", &self->model);

      shaderSet1i(self->shader, "u_texture", 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, self->texHandle);

      vboMakeCurrent(self->vbo);
      glDrawArrays(GL_TRIANGLES, 0, vboGetVertexCount(self->vbo));

   rendererPopCamera(r);
}
void fboBind(FBO *self) {
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, self->handle);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->texHandle, 0);
}