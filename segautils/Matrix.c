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

Float3 matrixMultiplyV(Matrix *lhs, Float3 rhs){
   return (Float3){
      lhs->data[0] * rhs.x + lhs->data[4] * rhs.y + lhs->data[8 ] * rhs.z + lhs->data[12],
      lhs->data[1] * rhs.x + lhs->data[5] * rhs.y + lhs->data[9 ] * rhs.z + lhs->data[13],
      lhs->data[2] * rhs.x + lhs->data[6] * rhs.y + lhs->data[10] * rhs.z + lhs->data[14]
   };
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

void matrixScale(Matrix *m, Float3 v) {
   Matrix scale;
   matrixIdentity(&scale);
   scale.data[0] = v.x;
   scale.data[5] = v.y;
   scale.data[10] = v.z;
   *m = matrixMultiply(m, &scale);
}

void matrixTranslate(Matrix *m, Float3 v) {
   Matrix trans;
   matrixIdentity(&trans);
   trans.data[12] = v.x;
   trans.data[13] = v.y;
   trans.data[14] = v.z;
   *m = matrixMultiply(m, &trans);

}
