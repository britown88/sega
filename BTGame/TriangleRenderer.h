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
   Int2 size, offset;

} Transform;