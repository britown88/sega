#include "MeshRendering.h"
#include "segalib\EGA.h"

#include "segalib\Rasterizer.h"
#include "segautils\BitTwiddling.h"
#include "segautils\Matrix.h"
#include "segautils\Quaternion.h"

#define VectorTPart Vertex
#include "segautils\Vector_Impl.h"
#include "SEGA\App.h"
#include <math.h>

typedef struct{
   FlatImage *img;
   Frame *frame;
} RenderData;

typedef struct{
   Int2 coords[3];
}TexCoords;

static void textureShader(RenderData *r, TexCoords *data, TrianglePoint *p){
   int i = 0;
   float texX = 0.0f, texY = 0.0f;
   int x, y;
   SuperScanLine *buff = NULL;

   if (p->pos.x < 0 || p->pos.x >= EGA_RES_WIDTH ||
      p->pos.y < 0 || p->pos.y >= EGA_RES_HEIGHT){
      return;
   }

   for (i = 0; i < 3; ++i){
      texX += data->coords[i].x * p->b[i];
      texY += data->coords[i].y * p->b[i];
   }

   x = abs((int)texX) % flatImageGetWidth(r->img);
   y = abs((int)texY) % flatImageGetHeight(r->img);

   buff = flatImageGetPlane(r->img, 0)->lines + y;
   if (getBitFromArray(buff->pixels, x)){
      for (i = 0; i < EGA_PLANES; ++i){
         buff = flatImageGetPlane(r->img, i + 1)->lines + y;

         setBitInArray(r->frame->planes[i].lines[p->pos.y].pixels, p->pos.x, getBitFromArray(buff->pixels, x));
      }
   }
}

static void buildTransform(Matrix *m, Transform t){
   Matrix m2 = quaternionToMatrix(&t.rotation);

   matrixIdentity(m);
   //matrixOrtho(m, 0.0f, EGA_RES_WIDTH, EGA_RES_HEIGHT, 0.0f, 1.0f, -1.0f);

   matrixTranslate(m, (Float3){ (float)t.offset.x, (float)t.offset.y, (float)t.offset.z });

   *m = matrixMultiply(m, &m2);

   matrixScale(m, (Float3){ (float)t.size.x, (float)t.size.y, (float)t.size.z });
}

void renderMesh(vec(Vertex) *vbo, vec(size_t) *ibo, Image *tex, Transform t, Frame *frame){
   RenderData r = { .img = imageRenderToFlat(tex) , .frame = frame};
   size_t iCount = vecSize(size_t)(ibo);
   size_t i = 0, j = 0;
   PixelShader shader = { 0 };
   Vertex *v[3] = { 0 };
   Float3 vPos[3] = { 0 };
   Matrix m;

   buildTransform(&m, t);
   closureInit(PixelShader)(&shader, &r, (PixelShaderFunc)&textureShader, NULL);

   for (i = 0; i < iCount / 3; ++i){
      Float3 n = { 0 };
      //fill out 3 vertices
      for (j = 0; j < 3; ++j){
         v[j] = vecAt(Vertex)(vbo, *vecAt(size_t)(ibo, i * 3 + j));
         vPos[j] = matrixMultiplyV(&m, v[j]->coords);
      }

      //determine if vertex is facing away from the screen
      n = vCross(vSubtract(vPos[1], vPos[0]), vSubtract(vPos[2], vPos[0]));

      if (n.z < 0.0f){
         //render
         TexCoords data = { .coords = { v[0]->texCoords, v[1]->texCoords, v[2]->texCoords } };
         drawTriangle(shader, &data, (Int2){ (int)vPos[0].x, (int)vPos[0].y },
                                     (Int2){ (int)vPos[1].x, (int)vPos[1].y },
                                     (Int2){ (int)vPos[2].x, (int)vPos[2].y });
      }
   }

   flatImageDestroy(r.img);
}