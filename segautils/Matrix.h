#pragma once

#include "Vector.h"

typedef struct {
   float data[16];
}Matrix;

Matrix matrixMultiply(Matrix *lhs, Matrix *rhs);
Float3 matrixMultiplyV(Matrix *lhs, Float3 rhs);
void matrixIdentity(Matrix *m);
void matrixOrtho(Matrix *m, float left, float right, float bottom, float top, float near, float far);
void matrixScale(Matrix *m, Float3 v);
void matrixTranslate(Matrix *m, Float3 v);

