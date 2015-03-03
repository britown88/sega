//#define ClosureT \
//    CLOSURE_RET(int) \
//    CLOSURE_NAME(SampleClosure) \
//    CLOSURE_ARGS(ClosureData, int, float)
//#include "Closure_Create.h"
//
//int foo(ClosureData a, int b, float c){
//   return 0;
//}
//
//SampleClosure c;
//closureInit(SampleClosure)(&c, data, &foo, &destructor);
//closureCall(&c, 1, 5.0f);
//closureDestroy(SampleClosure)(&c);

#define ClosureTPart ClosureT
#include "Closure_Decl.h"
#define ClosureTPart ClosureT
#include "Closure_Impl.h"
#undef ClosureT