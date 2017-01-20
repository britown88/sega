#pragma once

#include "segautils\Vector.h"
#include "segautils\StandardVectors.h"
#include "segautils\Quaternion.h"

typedef struct{
   Float3 coords;
   Float2 texCoords;
}Vertex;

#define VectorTPart Vertex
#include "segautils\Vector_Decl.h"

typedef struct{
   Int3 size, offset;
   Quaternion rotation;
} Transform;


typedef struct Texture_t Texture;
typedef struct FrameRegion_t FrameRegion;

void textureRenderMesh(Texture *frame, FrameRegion *vp, vec(Vertex) *vbo, vec(size_t) *ibo, Texture *tex, Transform modelTrans, Transform texTrans);