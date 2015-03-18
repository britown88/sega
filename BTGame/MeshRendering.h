#pragma once

#include "segautils\Vector.h"
#include "segautils\StandardVectors.h"

typedef struct{
   Float3 coords;
   Int2 texCoords;
}Vertex;

#define VectorTPart Vertex
#include "segautils\Vector_Decl.h"

typedef struct{
   Int3 size, offset;
} Transform;

typedef struct Image_t Image;
typedef struct Frame_t Frame;

void renderMesh(vec(Vertex) *vbo, vec(size_t) *ibo, Image *tex, Transform t, Frame *frame);