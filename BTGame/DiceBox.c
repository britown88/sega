#include "DiceBox.h"

#define VectorTPart TrayEdge
#include "segautils\Vector_Impl.h"

vec(TrayEdge) *diceTrayCreate(float width, float height){
   vec(TrayEdge) *out = vecCreate(TrayEdge)(NULL);

   TrayEdge base = {
      .plane = { .abc = {0.0f, 0.0f, 1.0f}, .d = 0.0f },
      .body = {
         .translation = { 0.0f, 0.0f, 0.0f },
         .centerOfMass = { width / 2.0f, height / 2.0f, 0.0f },
         .velocity = { 0.0f, 0.0f, 0.0f },
         .angularVelocity = { 0.0f, 0.0f, 0.0f },
         .orientation = quaternionUnit(),
         .invMass = 0.0f,
         .invInertialTensor = 0.0f
      }
   };

   vecPushBack(TrayEdge)(out, &base);
   return out;
}

static const float D6Mass = 1.0f;

RigidBody createD6Body(float size){
   return (RigidBody){
         .translation = { 0.0f, 0.0f, 0.0f },
         .centerOfMass = { 0.0f, 0.0f, 0.0f },
         .velocity = { 0.0f, 0.0f, 0.0f },
         .angularVelocity = { 0.0f, 0.0f, 0.0f },
         .orientation = quaternionUnit(),
         .invMass = 1.0f / D6Mass,
         .invInertialTensor = 6.0f / (D6Mass * size*size)
      };
}