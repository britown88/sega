#pragma once

#include "Defs.h"
#include "String.h"
#include "segashared/Strings.h"

#define LISP_EXPR(...) #__VA_ARGS__

typedef struct LispContext_t LispContext;

typedef struct  {
   void *type;
   void *data;
} LispExpr;

#define VectorTPart LispExpr
#include "Vector_Decl.h"

typedef vec(LispExpr)* LispList;
typedef StringView LispSym;
typedef String* LispString;

#define ClosureTPart \
    CLOSURE_RET(LispExpr) \
    CLOSURE_NAME(LispEvaluator) \
    CLOSURE_ARGS(LispExpr*, LispContext*)
#include "Closure_Decl.h"

LispExpr lispCreate();
LispExpr lispCreateStr(const char *str);
LispExpr lispCreateSym(LispSym str);
LispExpr lispCreatef32(float f);
LispExpr lispCreatei32(int i);
LispExpr lispCreateList(LispList list);
LispExpr lispCreateEval(LispEvaluator eval);

bool lispNil(LispExpr *self);
const char *lispStr(LispExpr *self);
LispSym *lispSym(LispExpr *self);
float *lispf32(LispExpr *self);
int *lispi32(LispExpr *self);
LispList *lispList(LispExpr *self);
LispEvaluator *lispEval(LispExpr *self);

void lispDestroy(LispExpr *self);

LispExpr lispParse(const char *script);

LispContext *lispContextCreate();
void lispContextDestroy(LispContext *self);

void lispContextPush(LispContext *self);
void lispContextPop(LispContext *self);
void lispContextStore(LispContext *self, LispSym key, LispExpr value);
LispExpr *lispContextLoad(LispContext *self, LispSym key);
LispExpr lispContextEvaluate(LispContext *self, LispExpr input);

