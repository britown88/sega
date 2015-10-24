 #include "Math.h"

int int2Dot(Int2 v1, Int2 v2) {
   return v1.x * v2.x + v1.y * v2.y;
}

Int2 int2Perp(Int2 v) {
   return (Int2) { -v.y, v.x };
}

Int2 int2Subtract(Int2 v1, Int2 v2) {
   return (Int2) { v1.x - v2.x, v1.y - v2.y };
}

int pointOnLine(Int2 l1, Int2 l2, Int2 point) {
   return int2Dot(int2Perp(int2Subtract(l2, l1)), int2Subtract(point, l1));
}

bool lineSegmentIntersectsAABBi(Int2 l1, Int2 l2, Recti *rect) {
   int topleft, topright, bottomright, bottomleft;

   if (l1.x > rect->right && l2.x > rect->right) { return false; }
   if (l1.x < rect->left && l2.x < rect->left) { return false; }
   if (l1.y > rect->bottom && l2.y > rect->bottom) { return false; }
   if (l1.y < rect->top && l2.y < rect->top) { return false; }

   topleft = pointOnLine(l1, l2, (Int2) { rect->left, rect->top });
   topright = pointOnLine(l1, l2, (Int2) { rect->right, rect->top });
   bottomright = pointOnLine(l1, l2, (Int2) { rect->right, rect->bottom });
   bottomleft = pointOnLine(l1, l2, (Int2) { rect->left, rect->bottom });

   if (topleft > 0 && topright > 0 && bottomright > 0 && bottomleft > 0) {
      return false;
   }

   if (topleft < 0 && topright < 0 && bottomright < 0 && bottomleft < 0) {
      return false;
   }

   return true;
   
}