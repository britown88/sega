#pragma once

#include "Defs.h"

typedef enum{
   Finished,
   NotFinished
}CoroutineStatus;

typedef enum{
   Continue,
   Cancel,
   ForceCancel,
   Pause
}CoroutineRequest;

static bool requestIsCancel(CoroutineRequest c){ return c == Cancel || c == ForceCancel; }

#define ClosureTPart \
    CLOSURE_RET(CoroutineStatus/*status*/) \
    CLOSURE_NAME(Coroutine) \
    CLOSURE_ARGS(CoroutineRequest/*cancel*/)
#include "Closure_Decl.h"

#define VectorTPart Coroutine
#include "Vector_Decl.h"

void coroutineDestroy(Coroutine *self);

Coroutine createSynchronizedList(vec(Coroutine) **listOut);
Coroutine createExecutionList(vec(Coroutine) **listOut);