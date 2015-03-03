//#define ClosureTPart \
//    CLOSURE_RET(int) \
//    CLOSURE_NAME(SampleClosure) \
//    CLOSURE_ARGS(int, float)
//#include "Closure_Decl.h"
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

//function typdef
#define CLOSURE_RET(TYPE) TYPE
#define CLOSURE_NAME(NAME) (*CONCAT(NAME, Func))
#define CLOSURE_ARGS(...) (ClosureData,__VA_ARGS__)

typedef ClosureTPart;

#undef CLOSURE_RET
#undef CLOSURE_NAME
#undef CLOSURE_ARGS

//struct
#define CLOSURE_RET(TYPE)
#define CLOSURE_NAME(NAME) NAME
#define CLOSURE_ARGS(...)

typedef struct{
   ClosureData data;
   CONCAT(ClosureTPart, Func) func;
   void(*destroy)(ClosureData);
}ClosureTPart;

//init and destroy
void CONCAT(ClosureTPart, Init)(ClosureTPart *self, ClosureData data, CONCAT(ClosureTPart, Func) func, void(*destroy)(ClosureData));
void CONCAT(ClosureTPart, Destroy)(ClosureTPart *self);

#undef CLOSURE_RET
#undef CLOSURE_NAME
#undef CLOSURE_ARGS
#undef ClosureTPart