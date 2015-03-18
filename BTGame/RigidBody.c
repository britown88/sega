#include "RigidBody.h"

Float3 contactOffset(RigidBody* body, Float3 contactPoint){
   return vSubtract(contactPoint, vAdd(body->translation, body->centerOfMass));
}

Float3 collisionVelocity(RigidBody* body, Float3 contactPoint){
   return vAdd(body->velocity, vCross(body->angularVelocity, contactOffset(body, contactPoint)));
}

Float3 relativeCollisionVelocity(RigidBody* b1, RigidBody* b2, Float3 contactPoint){
   return vSubtract(collisionVelocity(b2, contactPoint), collisionVelocity(b1, contactPoint));
}

Float3 impulseInertiaPart(RigidBody* body, Float3 contactPoint, Float3 contactNormal){
   Float3 offset = contactOffset(body, contactPoint);
   return vCross(vScale(vCross(offset, contactNormal), body->invInertialTensor), offset);
}


float getCollisionImpulse(RigidBody* b1, RigidBody* b2, Float3 contactPoint, Float3 contactNormal, float elasticity){
   float numer = vDot(vScale(relativeCollisionVelocity(b1, b2, contactPoint), -(1.0f + elasticity)), contactNormal);
   float denom = b1->invMass + b2->invMass + vDot(vAdd(impulseInertiaPart(b1, contactPoint, contactNormal), impulseInertiaPart(b2, contactPoint, contactNormal)), contactNormal);
   return numer / denom;
}


Float3 angularVelocityDelta(RigidBody* body, float impulse, Float3 contactPoint, Float3 contactNormal){
   Float3 offset = contactOffset(body, contactPoint);
   return vScale(vCross(offset, contactNormal), body->invInertialTensor * impulse);
}

void collideRigidBodies(RigidBody *b1, RigidBody *b2, Float3 contactPoint, Float3 contactNormal, float elasticity){
   float impulse = getCollisionImpulse(b1, b2, contactPoint, contactNormal, elasticity);
   Float3 impulseVec = vScale(contactNormal, impulse);

   //update velocity
   b1->velocity = vSubtract(b1->velocity, vScale(impulseVec, b1->invMass));
   b2->velocity = vAdd(b2->velocity, vScale(impulseVec, b2->invMass));

   //update angular velocity
   b1->angularVelocity = vSubtract(b1->angularVelocity, angularVelocityDelta(b1, impulse, contactPoint, contactNormal));
   b2->angularVelocity = vAdd(b2->angularVelocity, angularVelocityDelta(b2, impulse, contactPoint, contactNormal));
}