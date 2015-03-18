#include "Vector.h"

Float3 vCross(Float3 a, Float3 b){
   return (Float3){
      a.y*b.z - a.z*b.y,
      a.z*b.x - a.x*b.z,
      a.x*b.y - a.y*b.x
   };
}

float vDot(Float3 a, Float3 b){
   return a.x * b.x + a.y * b.y + a.z * b.z;
}
Float3 vSubtract(Float3 a, Float3 b){
   return (Float3){
      a.x - b.x,
      a.y - b.y,
      a.z - b.z,
   };
}
Float3 vAdd(Float3 a, Float3 b){
   return (Float3){
      a.x + b.x,
      a.y + b.y,
      a.z + b.z,
   };
}