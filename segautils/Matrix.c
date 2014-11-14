#include "Matrix.h"
#include <string.h>

Matrix matrixMultiply(Matrix *lhs, Matrix *rhs) {
   Matrix m;
   int x, y, i;
   for(y = 0; y < 4; ++y)
   {
      float v1[4] = {lhs->data[y], lhs->data[y + 4], lhs->data[y + 8], lhs->data[y + 12]};

      for(x = 0; x < 4; ++x)
      {
         const float *v2 = &(rhs->data[x * 4]);

         float v = 0.0f;
         for(i = 0; i < 4; ++i)
            v += v1[i] * v2[i];

         m.data[x*4 + y] = v;
      }
   }

   return m;
}

void matrixIdentity(Matrix *m) {
   memset((void*)m->data, 0, sizeof(Matrix));
   m->data[0] = 1.0f;
   m->data[5] = 1.0f;
   m->data[10] = 1.0f;
   m->data[15] = 1.0f;
}

void matrixOrtho(Matrix *m, float left, float right, float bottom, float top, float near, float far) {
   m->data[1] = m->data[2] = m->data[3] = m->data[4] = m->data[6] = 
      m->data[7] = m->data[8] = m->data[9] = m->data[11] = 0.0f;
   
   m->data[0 ] = 2.0f / (right - left);
   m->data[5 ] = 2.0f / (top - bottom);
   m->data[10] = -2.0f / (far - near);
   m->data[15] = 1.0f;

   m->data[12] = -((right + left) / (right - left));
   m->data[13] = -((top + bottom) / (top - bottom));
   m->data[14] = -((far + near) / (far - near));
}

void matrixScale(Matrix *m, float x, float y) {
   Matrix scale;
   matrixIdentity(&scale);
   scale.data[0] = x;
   scale.data[5] = y;
   scale.data[10] = 1.0f;
   *m = matrixMultiply(m, &scale);
}

void matrixTranslate(Matrix *m, float x, float y) {
   Matrix trans;
   matrixIdentity(&trans);
   trans.data[12] = x;
   trans.data[13] = y;
   trans.data[14] = 0.0f;
   *m = matrixMultiply(m, &trans);

}
