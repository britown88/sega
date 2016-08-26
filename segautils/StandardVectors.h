#pragma once

#include <stddef.h>
#include "Vector.h"
#include "String.h"

typedef String* StringPtr;
void stringPtrDestroy(StringPtr *self);
#define VectorTPart StringPtr
#include "Vector_Decl.h"

#define VectorTPart size_t
#include "Vector_Decl.h"

#define VectorTPart char
#include "Vector_Decl.h"

#define VectorTPart byte
#include "Vector_Decl.h"

#define VectorTPart int
#include "Vector_Decl.h"

#define VectorTPart float
#include "Vector_Decl.h"

#define VectorTPart Int2
#include "Vector_Decl.h"

#define VectorTPart Int3
#include "Vector_Decl.h"

#define VectorTPart Float2
#include "Vector_Decl.h"

#define VectorTPart Float3
#include "Vector_Decl.h"
