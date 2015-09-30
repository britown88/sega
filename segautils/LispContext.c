#include "Lisp.h"

struct LispContext_t {
   int foo;
};

LispContext *lispContextCreate() {}
void lispContextDestroy(LispContext *self) {}

void lispContextPush(LispContext *self) {}
void lispContextPop(LispContext *self) {}
void lispContextStore(LispContext *self, LispSym key, LispExpr value) {}
LispExpr *lispContextLoad(LispContext *self, LispSym key) {}
LispExpr lispContextEvaluate(LispContext *self, LispExpr input) {}