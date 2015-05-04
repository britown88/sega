#pragma once
#include "Defs.h"

typedef struct {
   float left, top, right, bottom;
} Rectf;

static Rectf rectfCreate(float left, float top, float right, float bottom) {
   Rectf r = {left, top, right, bottom};
   return r;
}
static float rectfWidth(Rectf *r) {
   return r->right - r->left;
}
static float rectfHeight(Rectf *r) {
   return r->bottom - r->top;
}
static void rectfOffset(Rectf *r, float x, float y) {
   r->left += x;
   r->right += x;
   r->top += y;
   r->bottom += y;
}

typedef struct {
   int left, top, right, bottom;
} Recti;

static Recti rectiCreate(int left, int top, int right, int bottom) {
   Recti r = {left, top, right, bottom};
   return r;
}

static bool rectiIntersects(Recti a, Recti b){
   if (a.left >= b.right ||
      a.top >= b.bottom ||
      b.left >= a.right ||
      b.top >= a.bottom) return false;
   return true;
}