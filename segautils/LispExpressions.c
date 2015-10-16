#include "Lisp.h"

const LispExpr LispNil = { NULL, NULL };

#define VectorTPart LispExpr
#include "Vector_Impl.h"

#define ClosureTPart CLOSURE_NAME(LispEvaluator)
#include "Closure_Impl.h"

typedef struct {
   size_t(*size)();
   void(*destroy)(void*);
}LispTypeVTable;

#define RTTI_VTABLE_GET(Name) CONCAT(__vtableGet_, Name)()

#define RTTI_VTABLE(Name, DestroyFunc) \
   static size_t CONCAT(__vtableSize_, Name)() {return sizeof(Name); } \
   static void CONCAT(__vtableDestroy_, Name)(void *self) DestroyFunc \
   static LispTypeVTable *RTTI_VTABLE_GET(Name) { \
      static LispTypeVTable *out = NULL; \
      if (!out) { \
         out = calloc(1, sizeof(LispTypeVTable)); \
         out->size = &CONCAT(__vtableSize_, Name); \
         out->destroy = &CONCAT(__vtableDestroy_, Name); \
      } \
      return out; \
   }

//really silly rtti
RTTI_VTABLE(int, {})
RTTI_VTABLE(float, {})
RTTI_VTABLE(LispString, { stringDestroy(self); })
RTTI_VTABLE(LispSym, {})
RTTI_VTABLE(LispList, { vecDestroy(LispExpr)(self); })
RTTI_VTABLE(LispEvaluator, { closureDestroy(LispEvaluator)(self); })

static LispExpr _lispCreate(LispTypeVTable *vtable, void *data) {
   size_t size = vtable->size();
   LispExpr out = (LispExpr) { vtable, NULL };

   if (size > sizeof(out.data)) {
      out.data = checkedCalloc(1, size);
      memcpy(out.data, data, size);
   }
   else{
      memcpy(&out.data, data, size);
   }

   return out;
}

static void *_lispExprGetData(LispExpr *self) {
   if (!lispNil(self)) {
      if (((LispTypeVTable*)self->type)->size() > sizeof(self->data)) {
         return self->data;
      }
      else {
         return &self->data;
      }
   }
   return NULL;
}

LispExpr lispCreate() { return (LispExpr){ NULL, NULL }; }
LispExpr lispCreateStr(const char *str) { LispString s = stringCreate(str);  return _lispCreate(RTTI_VTABLE_GET(LispString), &s); }
LispExpr lispCreateSym(LispSym str) { return _lispCreate(RTTI_VTABLE_GET(LispSym), &str); }
LispExpr lispCreatef32(float f) { return _lispCreate(RTTI_VTABLE_GET(float), &f); }
LispExpr lispCreatei32(int i) { return _lispCreate(RTTI_VTABLE_GET(int), &i); }
LispExpr lispCreateList(LispList list) { return _lispCreate(RTTI_VTABLE_GET(LispList), &list); }
LispExpr lispCreateEval(LispEvaluator eval) { return _lispCreate(RTTI_VTABLE_GET(LispEvaluator), &eval); }

static void *_lispResolve(LispTypeVTable *vtable, LispExpr *self) {
   if (self && (LispTypeVTable*)self->type == vtable) {
      return _lispExprGetData(self);
   }
   return NULL;
}

bool lispNil(LispExpr *self) { return !self || !self->type; }
const char *lispStr(LispExpr *self) { 
   LispString *str = _lispResolve(RTTI_VTABLE_GET(LispString), self);
   return str ? c_str(*str) : NULL;
}
LispSym *lispSym(LispExpr *self) { return _lispResolve(RTTI_VTABLE_GET(LispSym), self); }
float *lispf32(LispExpr *self) { return _lispResolve(RTTI_VTABLE_GET(float), self); }
int *lispi32(LispExpr *self) { return _lispResolve(RTTI_VTABLE_GET(int), self); }
LispList *lispList(LispExpr *self) { return _lispResolve(RTTI_VTABLE_GET(LispList), self); }
LispEvaluator *lispEval(LispExpr *self) { return _lispResolve(RTTI_VTABLE_GET(LispEvaluator), self); }

void lispDestroy(LispExpr *self) {
   if (!lispNil(self)) {
      LispTypeVTable *vt = self->type;

      vt->destroy(_lispExprGetData(self));

      if (vt->size() > sizeof(self->data)) {
         checkedFree(self->data);
      }

      self->data = self->type = NULL;
   }
}



