#pragma once

#include "Vector.h"
#include "Matrix.h"

typedef struct Quaternion_t {
   Float3 xyz;
   float w;
}Quaternion;

Quaternion quaternionFromAxisAngle(Float3 axis, float angle);
Quaternion quaternionFromAngularVelocity(Float3 v);
Quaternion quaternionUnit();
Matrix quaternionToMatrix(Quaternion *q);