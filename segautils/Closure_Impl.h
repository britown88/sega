//#define ClosureTPart CLOSURE_NAME(SampleClosure)
//#include "Closure_Impl.h"
//
//int foo(ClosureData a, int b, float c){
//   return 0;
//}
//
//SampleClosure c;
//closureInit(SampleClosure)(&c, data, &foo, &destructor);
//closureCall(&c, 1, 5.0f);
//closureDestroy(SampleClosure)(&c);

#include "Closure_Functions.h"
#include <string.h>

//struct
#define CLOSURE_RET(TYPE)
#define CLOSURE_NAME(NAME) NAME
#define CLOSURE_ARGS(...)

//init and destroy
void CONCAT(ClosureTPart, Init)(ClosureTPart *self, ClosureData data, CONCAT(ClosureTPart, Func) func, void(*destroy)(ClosureData)){
   self->data = data;
   self->func = func;
   self->destroy = destroy;
}

void CONCAT(ClosureTPart, Destroy)(ClosureTPart *self){
   if (self->destroy){
      self->destroy(self->data);
   }
}

bool CONCAT(ClosureTPart, IsNull)(ClosureTPart *self){
   static ClosureTPart empty = { 0 };
   return memcmp(self, &empty, sizeof(ClosureTPart)) == 0;
}

#undef CLOSURE_RET
#undef CLOSURE_NAME
#undef CLOSURE_ARGS
#undef ClosureTPart

