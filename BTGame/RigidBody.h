#pragma once

#include "segautils\Vector.h"
#include "segautils\Quaternion.h"

typedef struct {
   Float3 translation;
   Float3 centerOfMass;
   Float3 velocity;
   Float3 angularVelocity;
   Quaternion orientation;
   float invMass;  //(1/mass, 0 for objects that can't move.)
   float invInertialTensor;
} RigidBody;

void collideRigidBodies(RigidBody *b1, RigidBody *b2, Float3 contactPoint, Float3 contactNormal, float elasticity);