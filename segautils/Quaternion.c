#include "Quaternion.h"
#include <math.h>

Quaternion quaternionFromAxisAngle(Float3 axis, float angle){
   float halfAngle = angle / 2;
   float sinHalfAngle = (float)sin(halfAngle);

   return (Quaternion){
      .xyz = (Float3){
         .x = axis.x * sinHalfAngle,
         .y = axis.y * sinHalfAngle,
         .z = axis.z * sinHalfAngle },
      .w = (float)cos(halfAngle)
   };
}

Quaternion quaternionFromAngularVelocity(Float3 v){
   float l = sqrtf(vDot(v, v));
   return (Quaternion){ {0.0f, 0.0f, 0.0f}, 1.0f };
}

Quaternion quaternionUnit(){
   return (Quaternion){ {0.0f, 0.0f, 0.0f}, 1.0f };
}

Matrix quaternionToMatrix(Quaternion *q){
   float qx = q->xyz.x;
   float qy = q->xyz.y;
   float qz = q->xyz.z;
   float qw = q->w;

   return (Matrix){
      1 - 2 * qy*qy - 2 * qz*qz, 2 * qx*qy - 2 * qz*qw,     2 * qx*qz + 2 * qy*qw,     0.0f,
      2 * qx*qy + 2 * qz*qw,     1 - 2 * qx*qx - 2 * qz*qz, 2 * qy*qz - 2 * qx*qw,     0.0f,
      2 * qx*qz - 2 * qy*qw,     2 * qy*qz + 2 * qx*qw,     1 - 2 * qx*qx - 2 * qy*qy, 0.0f,
      0.0f,                      0.0f,                      0.0f,                      1.0f
   };
}