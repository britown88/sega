#pragma once

#include "segautils\Plane.h"
#include "RigidBody.h"

typedef struct{
   RigidBody body;
   Plane plane;
}TrayEdge;

#define VectorTPart TrayEdge
#include "segautils\Vector_Decl.h"

vec(TrayEdge) *diceTrayCreate(float width, float height);

RigidBody createD6Body(float size);