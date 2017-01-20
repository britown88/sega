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
   Texture *img;
   Texture *frame;
   FrameRegion *region;
} RenderData;

typedef struct{
   Float2 coords[3];
}TexCoords;

static void textureShader(RenderData *r, TexCoords *data, TrianglePoint *p){
   int i = 0;
   float texX = 0.0f, texY = 0.0f;
   int tx, ty, fx, fy;
   byte *buff;

   int texWidth = textureGetWidth(r->img);
   int texHeight = textureGetHeight(r->img);   
   int frameWidth = textureGetWidth(r->frame);
   int frameHeight = textureGetHeight(r->frame);

   fx = p->pos.x;
   fy = p->pos.y;

   if (fx < 0 || fx >= r->region->width || fy < 0 || fy >= r->region->height){
      return;
   }

   for (i = 0; i < 3; ++i){
      texX += data->coords[i].x * p->b[i];
      texY += data->coords[i].y * p->b[i];
   }

   tx = abs((int)texX) % texWidth;
   ty = abs((int)texY) % texHeight;

   fx += r->region->origin_x;
   fy += r->region->origin_y;

   if (fx < 0 || fx >= frameWidth ||
      fy < 0 || fy >= frameHeight) {
      return;
   }

   buff = textureGetAlphaScanline(r->img, ty);
   if (!getBitFromArray(buff, tx)){
      setBitInArray(textureGetAlphaScanline(r->frame, fy), fx, 0);
      for (i = 0; i < EGA_PLANES; ++i){
         buff = textureGetScanline(r->img, i, ty);
         setBitInArray(textureGetScanline(r->frame, i, fy), fx, getBitFromArray(buff, tx));
      }
   }
}

static void pixelShader(RenderData *r, TexCoords *data, TrianglePoint *p){
   textureRenderPoint(r->frame, NULL, p->pos.x, p->pos.y, 15);
}

static void buildTransform(Matrix *m, Transform t) {
   Matrix m2 = quaternionToMatrix(&t.rotation);

   matrixIdentity(m);
   //matrixOrtho(m, 0.0f, EGA_RES_WIDTH, EGA_RES_HEIGHT, 0.0f, 1.0f, -1.0f);

   matrixTranslate(m, (Float3) { (float)t.offset.x, (float)t.offset.y, (float)t.offset.z });
   //*m = matrixMultiply(m, &m2);
   matrixScale(m, (Float3) { (float)t.size.x, (float)t.size.y, (float)t.size.z });
}

static Float2 _texTransform(Float2 *in, Matrix *m) {
   Float3 out = matrixMultiplyV(m, (Float3) { in->x, in->y, 0.0f });
   return (Float2) {out.x, out.y};
}

void textureRenderMesh(Texture *frame, FrameRegion *vp, vec(Vertex) *vbo, vec(size_t) *ibo, Texture *tex, Transform modelTrans, Transform texTrans) {

   RenderData r = { .img = tex, .frame = frame, .region = vp ? vp : textureGetFullRegion(tex) };
   size_t iCount = vecSize(size_t)(ibo);
   size_t i = 0, j = 0;
   PixelShader shader = { 0 };
   Vertex *v[3] = { 0 };
   Float3 vPos[3] = { 0 };
   Matrix m, tm;

   buildTransform(&m, modelTrans);
   buildTransform(&tm, texTrans);

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
         TexCoords data = { 
            .coords = { 
               _texTransform(&v[0]->texCoords, &tm),
               _texTransform(&v[1]->texCoords, &tm),
               _texTransform(&v[2]->texCoords, &tm),
            } 
         };

         drawTriangle(shader, &data, (Int2){ (int)vPos[0].x, (int)vPos[0].y },
                                     (Int2){ (int)vPos[1].x, (int)vPos[1].y },
                                     (Int2){ (int)vPos[2].x, (int)vPos[2].y });
      }
   }
}