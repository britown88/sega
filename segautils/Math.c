#include "Math.h"

int pointOnLine(Int2 l1, Int2 l2, Int2 point) {
   return (l2.y - l1.y) * point.x + (l1.x - l2.x) * point.y + (l2.x * l1.y - l1.x * l2.y);
}

bool lineSegmentIntersectsAABBi(Int2 l1, Int2 l2, Recti *rect) {
   enum {
      above, 
      below,
      on
   };

   int dir = 0;
   int online;
   bool colPossible = false;

   online = pointOnLine(l1, l2, (Int2) { rect->left, rect->top });
   if (online > 0) {
      dir = above;
   }
   else if(online < 0){
      dir = below;
   }

   online = pointOnLine(l1, l2, (Int2) { rect->right, rect->top });
   if (online > 0) {
      if (dir == below) {
         colPossible = true;
      }
   } 
   else if (online < 0 && dir == above) {
      colPossible = true;
   }

   online = pointOnLine(l1, l2, (Int2) { rect->right, rect->bottom });
   if (!colPossible && online > 0) {
      if (dir == below) {
         colPossible = true;
      }
   }
   else if (online < 0 && dir == above) {
      colPossible = true;
   }

   online = pointOnLine(l1, l2, (Int2) { rect->left, rect->bottom });
   if (!colPossible && online > 0) {
      if (dir == below) {
         colPossible = true;
      }
   }
   else if (online < 0 && dir == above) {
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