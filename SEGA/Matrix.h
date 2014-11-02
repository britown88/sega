#pragma once

typedef struct {
   float data[16];
}Matrix;

Matrix matrixMultiply(Matrix *lhs, Matrix *rhs);
void matrixIdentity(Matrix *m);
void matrixOrtho(Matrix *m, float left, float right, float bottom, float top, float near, float far);
void matrixScale(Matrix *m, float x, float y);
void matrixTranslate(Matrix *m, float x, float y);

