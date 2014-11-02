#pragma once

#include "Shader.h"
#include "Defs.h"

typedef struct VBO_t VBO;



void vboAddAttribute(VBO *self, e_shaderAttributes attr, bool persistAfterBind);
void vboBuild(VBO *self, unsigned int vertexCount);

VBO *vboCreate();
void vboDestroy(VBO *self);

unsigned int vboGetVertexCount(VBO *self);
void *vboGetAttributeChunk(VBO *self, e_shaderAttributes attr);
void vboSetSingleVertex(VBO *self, e_shaderAttributes attr, unsigned int vIndex, void *data);
//unsigned int vboGetGLHandle(VBO *self); //not public for now, gets called by makecurrent
void vboPush(VBO *self);
void vboMakeCurrent(VBO *self);
