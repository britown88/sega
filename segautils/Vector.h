#pragma once

typedef struct {
   int x, y;
} Int2;

static Int2 int2Create(int x, int y){
   Int2 r;
   r.x = x;
   r.y = y;
   return r;
}

typedef struct {
   float x, y;
} Float2;

static Float2 float2Create(float x, float y){
   Float2 r;
   r.x = x;
   r.y = y;
   return r;
}