
#pragma once

typedef struct {
   int x, y;
} Int2;

typedef struct {
   int x, y, z;
} Int3;

typedef struct {
   float x, y;
} Float2;

typedef struct {
   float x, y, z;
} Float3;

Float3 vCross(Float3 a, Float3 b);
float vDot(Float3 a, Float3 b);
Float3 vSubtract(Float3 a, Float3 b);
Float3 vAdd(Float3 a, Float3 b);
Float3 vNormalized(Float3 v);
Float3 *vNormalize(Float3 *v);
Float3 vScale(Float3 v, float s);