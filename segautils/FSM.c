#include "FSM.h"

#define ClosureTPart CLOSURE_NAME(StateClosure)
#include "Closure_Impl.h"

#define VectorT StateClosure
#include "Vector_Create.h"

ImpleStateMessage(StateEnter)
ImpleStateMessage(StateExit)

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
   fsmClear(self);
   vecDestroy(StateClosure)(self->states);
   checkedFree(self);
}

void fsmPush(FSM *self, StateClosure state){
   fsmSend(self, StateExit);
   vecPushBack(StateClosure)(self->states, &state);
   fsmSend(self, StateEnter);
}
void fsmPop(FSM *self){
   if (!fsmIsEmpty(self)){
      fsmSend(self, StateExit);
      vecPopBack(StateClosure)(self->states);
      fsmSend(self, StateEnter);
   }   
}
void fsmSet(FSM *self, StateClosure state){
   if (!fsmIsEmpty(self)) {
      vecPopBack(StateClosure)(self->states);
   }
   vecPushBack(StateClosure)(self->states, &state);
   fsmSend(self, StateEnter);
}
bool fsmIsEmpty(FSM *self){
   return vecIsEmpty(StateClosure)(self->states);
}
void fsmClear(FSM *self){
   while (!fsmIsEmpty(self)) {
      fsmPop(self);
   }
   vecClear(StateClosure)(self->states);
}
void fsmSendEx(FSM *self, Type *t, Message m){
   if (!fsmIsEmpty(self)){
      closureCall(vecBack(StateClosure)(self->states), t, m);
   }
}