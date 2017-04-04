#pragma once

#include "Vector.h"
#include "Rect.h"

#define PI  3.14159265358979323846
#define TAU (2 * PI)

bool lineSegmentIntersectsAABBi(Int2 l1, Int2 l2, Recti *rect);