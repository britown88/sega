#include "Lisp.h"

struct LispContext_t {
   int foo;
};

LispContext *lispContextCreate() { return NULL; }
void lispContextDestroy(LispContext *self) {}

void lispContextPush(LispContext *self) {}
void lispContextPop(LispContext *self) {}
void lispContextStore(LispContext *self, LispSym key, LispExpr value) {}
LispExpr *lispContextLoad(LispContext *self, LispSym key) { return NULL; }
LispExpr lispContextEvaluate(LispContext *self, LispExpr input) { return LispNil; }