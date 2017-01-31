#pragma once

#include "Vector.h"
#include "Rect.h"

#define PI 3.1415926535

bool lineSegmentIntersectsAABBi(Int2 l1, Int2 l2, Recti *rect);