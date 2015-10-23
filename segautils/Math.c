#include "Math.h"

int pointOnLine(Int2 l1, Int2 l2, Int2 point) {
   return (l2.y - l1.y) * point.x + (l1.x - l2.x) * point.y + (l2.x * l1.y - l1.x * l2.y);
}

bool lineSegmentIntersectsAABBi(Int2 l1, Int2 l2, Recti *rect) {
   enum {
      above, 
      below
   };

   int dir = 0;
   bool colPossible = false;

   if (pointOnLine(l1, l2, (Int2) { rect->left, rect->top }) >= 0) {
      dir = above;
   }
   else {
      dir = below;
   }

   if (pointOnLine(l1, l2, (Int2) { rect->right, rect->top }) >= 0) { 
      if (dir == below) {
         colPossible = true;
      }
   } 
   else if (dir == above) {
      colPossible = true;
   }

   if (!colPossible && pointOnLine(l1, l2, (Int2) { rect->right, rect->bottom }) >= 0) {
      if (dir == below) {
         colPossible = true;
      }
   }
   else if (dir == above) {
      colPossible = true;
   }

   if (!colPossible && pointOnLine(l1, l2, (Int2) { rect->left, rect->bottom }) >= 0) {
      if (dir == below) {
         colPossible = true;
      }
   }
   else if (dir == above) {
      colPossible = true;
   }

   if (colPossible) {
      if (l1.x > rect->right && l2.x > rect->right) { return false; }
      if (l1.x < rect->left && l2.x < rect->left) { return false; }
      if (l1.y > rect->bottom && l2.y > rect->bottom) { return false; }
      if (l1.y < rect->top && l2.y < rect->top) { return false; }

      return true;
   }

   return false;
}