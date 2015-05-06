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

Coroutine createSynchronizedList(vec(Coroutine) **listOut);
Coroutine createExecutionList(vec(Coroutine) **listOut);