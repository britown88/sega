#pragma once

#include "segashared\RTTI.h"

typedef struct FSM_t FSM;

typedef void* Message;

#define ClosureTPart \
    CLOSURE_RET(void) \
    CLOSURE_NAME(StateClosure) \
    CLOSURE_ARGS(Type *, Message)
#include "Closure_Decl.h"

FSM *fsmCreate();
void fsmDestroy(FSM *self);

void fsmPush(FSM *self, StateClosure state);
void fsmPop(FSM *self);
void fsmSet(FSM *self, StateClosure state);
bool fsmIsEmpty(FSM *self);
void fsmClear(FSM *self);
void fsmSendEx(FSM *self, Type *t, Message m);

#define fsmSendData(fsm, TypeName, ...) fsmSendEx(fsm, GetRTTI(TypeName), &(TypeName){ __VA_ARGS__ })
#define fsmSend(fsm, TypeName) fsmSendEx(fsm, GetRTTI(TypeName), &(TypeName){ 0 })

#define DeclStateMessage(TypeName) \
   typedef struct { EMPTY_STRUCT; }TypeName; \
   DeclRTTI(TypeName)

#define DeclStateMessageWithData(TypeName, ...) \
   typedef struct __VA_ARGS__ TypeName; \
   DeclRTTI(TypeName)

#define ImpleStateMessage(TypeName) ImplRTTI(TypeName)