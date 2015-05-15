#pragma once

#include "Defs.h"

typedef enum{
   Finished,
   NotFinished
}CoroutineStatus;

#define ClosureTPart \
    CLOSURE_RET(CoroutineStatus/*status*/) \
    CLOSURE_NAME(Coroutine) \
    CLOSURE_ARGS(bool/*cancel*/)
#include "Closure_Decl.h"

#define VectorTPart Coroutine
#include "Vector_Decl.h"

void coroutineDestroy(Coroutine *self);

Coroutine createSynchronizedList(vec(Coroutine) **listOut);
Coroutine createExecutionList(vec(Coroutine) **listOut);

Coroutine coroutineNull();
bool coroutineIsNull(Coroutine c);