#include "FSM.h"

#define ClosureTPart CLOSURE_NAME(StateClosure)
#include "Closure_Impl.h"

#define VectorT StateClosure
#include "Vector_Create.h"

struct FSM_t{
   vec(StateClosure) *states;
};

static void stateClosureDestroy(StateClosure *self){
   closureDestroy(StateClosure)(self);
}

FSM *fsmCreate(){
   FSM *r = checkedCalloc(1, sizeof(FSM));
   r->states = vecCreate(StateClosure)(&stateClosureDestroy);
   return r;
}
void fsmDestroy(FSM *self){
   vecDestroy(StateClosure)(self->states);
   checkedFree(self);
}

void fsmPush(FSM *self, StateClosure state){
   vecPushBack(StateClosure)(self->states, &state);
}
void fsmPop(FSM *self){
   if (!fsmIsEmpty(self)){
      vecPopBack(StateClosure)(self->states);
   }   
}
void fsmSet(FSM *self, StateClosure state){
   fsmPop(self);
   fsmPush(self, state);
}
bool fsmIsEmpty(FSM *self){
   return vecIsEmpty(StateClosure)(self->states);
}
void fsmClear(FSM *self){
   vecClear(StateClosure)(self->states);
}
void fsmSendEx(FSM *self, Type *t, Message m){
   if (!fsmIsEmpty(self)){
      closureCall(vecBack(StateClosure)(self->states), t, m);
   }
}