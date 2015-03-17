#pragma once

#include "segautils\Vector.h"

typedef struct{
   Int2 pos;
   float b[3];
}TrianglePoint;

#define ClosureTPart \
    CLOSURE_RET(void) \
    CLOSURE_NAME(PixelShader) \
    CLOSURE_ARGS(TrianglePoint*)
#include "segautils\Closure_Decl.h"

void drawTriangle(PixelShader shader, Int2 p1, Int2 p2, Int2 p3);