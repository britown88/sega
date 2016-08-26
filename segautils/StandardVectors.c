#include "StandardVectors.h"

void stringPtrDestroy(StringPtr *self) {
   stringDestroy(*self);
}
#define VectorTPart StringPtr
#include "Vector_Impl.h"

#define VectorTPart size_t
#include "Vector_Impl.h"

#define VectorTPart char
#include "Vector_Impl.h"

#define VectorTPart byte
#include "Vector_Impl.h"

#define VectorTPart int
#include "Vector_Impl.h"

#define VectorTPart float
#include "Vector_Impl.h"

#define VectorTPart Int2
#include "Vector_Impl.h"

#define VectorTPart Int3
#include "Vector_Impl.h"

#define VectorTPart Float2
#include "Vector_Impl.h"

#define VectorTPart Float3
#include "Vector_Impl.h"