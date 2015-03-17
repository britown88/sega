#include "Rasterizer.h"

#define ClosureTPart \
    CLOSURE_RET(void) \
    CLOSURE_NAME(PixelShader) \
    CLOSURE_ARGS(TrianglePoint*)
#include "segautils\Closure_Impl.h"

static void sortTriangles(TrianglePoint p1, TrianglePoint p2, TrianglePoint p3, TrianglePoint *points){
   if (p1.pos.y < p2.pos.y){
      if (p1.pos.y < p3.pos.y){
         points[0] = p1;
         if (p2.pos.y < p3.pos.y){
            points[1] = p2;
            points[2] = p3;
         }
         else{
            points[1] = p3;
            points[2] = p2;
         }
      }
      else{
         points[0] = p3;
         points[1] = p1;
         points[2] = p2;
      }
   }
   else{
      if (p2.pos.y < p3.pos.y){
         points[0] = p2;
         if (p1.pos.y < p3.pos.y){
            points[1] = p1;
            points[2] = p3;
         }
         else{
            points[1] = p3;
            points[2] = p1;
         }
      }
      else{
         points[0] = p3;
         points[1] = p2;
         points[2] = p1;
      }
   }
}

static void drawFlatTop(PixelShader shader, TriangleData data, TrianglePoint *points){
   TrianglePoint bottom, left, right;
   float dxLeft = 0.0f, dxRight = 0.0f;
   int i = 0, j = 0, k = 0;
   float leftSide = 0.0f, rightSide = 0.0f;
   float bLeft[3] = { 0 }, bRight[3] = { 0 },
      dybLeft[3] = { 0 }, dybRight[3] = { 0 },
      dxb[3] = { 0 };

   float idy = 0.0f, idx = 0.0f;

   bottom = points[2];
   if (points[0].pos.x < points[1].pos.x){
      left = points[0];
      right = points[1];
   }
   else{
      left = points[1];
      right = points[0];
   }

   idy = 1.0f / (bottom.pos.y - left.pos.y);
   idx = 1.0f / (right.pos.x - left.pos.x);

   dxLeft = (left.pos.x - bottom.pos.x) * idy;
   dxRight = (right.pos.x - bottom.pos.x) * idy;

   for (i = 0; i < 3; ++i){      
      dybLeft[i] = (left.b[i] - bottom.b[i]) * idy;
      dybRight[i] = (right.b[i] - bottom.b[i]) * idy;

      dxb[i] = (right.b[i] - left.b[i]) * idx;

      bLeft[i] = bottom.b[i];
      bRight[i] = bottom.b[i];
   }

   leftSide = rightSide = (float)bottom.pos.x;

   for (i = bottom.pos.y; i >= left.pos.y; --i){  
      //draw the fucking scanline
      float b[3] = { bLeft[0], bLeft[1], bLeft[2]};
      for (j = (int)leftSide; j <= (int)rightSide; ++j){
         closureCall(&shader, data, &(TrianglePoint){
               .pos = {.x = j, .y = i},
               .b = {b[0], b[1], b[2]}
            });

         for (k = 0; k < 3; ++k){
            b[k] += dxb[k];
         }
      }      

      leftSide += dxLeft;
      rightSide += dxRight;

      for (j = 0; j < 3; ++j){
         bLeft[j] += dybLeft[j];
         bRight[j] += dybRight[j];
      }
   }
}

static void drawFlatBottom(PixelShader shader, TriangleData data, TrianglePoint *points){
   TrianglePoint top, left, right;
   float dxLeft = 0.0f, dxRight = 0.0f;
   int i = 0, j = 0, k = 0;
   float leftSide = 0.0f, rightSide = 0.0f;
   float bLeft[3] = { 0 }, bRight[3] = { 0 },
      dybLeft[3] = { 0 }, dybRight[3] = { 0 },
      dxb[3] = { 0 };

   float idy = 0.0f, idx = 0.0f;

   top = points[0];
   if (points[1].pos.x < points[2].pos.x){
      left = points[1];
      right = points[2];
   }
   else{
      left = points[2];
      right = points[1];
   } 

   idy = 1.0f / (left.pos.y - top.pos.y);
   idx = 1.0f / (right.pos.x - left.pos.x);

   dxLeft = (left.pos.x - top.pos.x) * idy;
   dxRight = (right.pos.x - top.pos.x) * idy;

   for (i = 0; i < 3; ++i){
      dybLeft[i] = (left.b[i] - top.b[i]) * idy;
      dybRight[i] = (right.b[i] - top.b[i]) * idy;

      dxb[i] = (right.b[i] - left.b[i]) * idx;

      bLeft[i] = top.b[i];
      bRight[i] = top.b[i];
   }

   leftSide = rightSide = (float)top.pos.x;

   for (i = top.pos.y; i <= left.pos.y; ++i){

      float b[3] = { bLeft[0], bLeft[1], bLeft[2] };
      for (j = (int)leftSide; j <= (int)rightSide; ++j){
         closureCall(&shader, data, &(TrianglePoint){
               .pos = { .x = j, .y = i },
               .b = { b[0], b[1], b[2] }
         });

         for (k = 0; k < 3; ++k){
            b[k] += dxb[k];
         }
      }

      leftSide += dxLeft;
      rightSide += dxRight;

      for (j = 0; j < 3; ++j){
         bLeft[j] += dybLeft[j];
         bRight[j] += dybRight[j];
      }
   }
}

static TrianglePoint getSplitPoint(TrianglePoint *points){
   float idy = 1.0f / (points[2].pos.y - points[0].pos.y);
   float dx = (points[2].pos.x - points[0].pos.x) * idy;
   int i = 0;
   
   float db[3] = {
      (points[2].b[0] - points[0].b[0]) * idy,
      (points[2].b[1] - points[0].b[1]) * idy,
      (points[2].b[2] - points[0].b[2]) * idy
   };
   TrianglePoint out;

   out.pos.x = points[0].pos.x + (int)(dx * (points[1].pos.y - points[0].pos.y));
   out.pos.y = points[1].pos.y;

   for (i = 0; i < 3; ++i){
      out.b[i] = points[0].b[i] + (float)(db[i] * (points[1].pos.y - points[0].pos.y));
   }

   return out;
}

static void drawSplit(PixelShader shader, TriangleData data, TrianglePoint *points){
   TrianglePoint top[3];
   TrianglePoint bottom[3];
   TrianglePoint splitPoint = getSplitPoint(points);

   top[0] = points[0];
   top[1] = points[1];
   top[2] = splitPoint;

   bottom[0] = points[1];
   bottom[1] = splitPoint;
   bottom[2] = points[2];

   drawFlatTop(shader, data, bottom);
   drawFlatBottom(shader, data, top);
}

void drawTriangle(PixelShader shader, TriangleData data, Int2 p1, Int2 p2, Int2 p3){
   TrianglePoint points[3] = {
      { p1, { 1.0f, 0.0f, 0.0f } },
      { p2, { 0.0f, 1.0f, 0.0f } },
      { p3, { 0.0f, 0.0f, 1.0f } }
   };

   sortTriangles(points[0], points[1], points[2], points);

   if (points[0].pos.y == points[1].pos.y){
      //fat top
      drawFlatTop(shader, data, points);
   }
   else if (points[1].pos.y == points[2].pos.y){
      //fat bottom
      drawFlatBottom(shader, data, points);
   }
   else{
      drawSplit(shader, data, points);
   }
}