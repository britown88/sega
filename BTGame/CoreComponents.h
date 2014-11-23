#pragma once

#include "Entities\Entities.h"

typedef struct {
   int x, y;
}PositionComponent;

#define ComponentT PositionComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   int x, y;
}VelocityComponent;

#define ComponentT VelocityComponent
#include "Entities\ComponentDecl.h"

