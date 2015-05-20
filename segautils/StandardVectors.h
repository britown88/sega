#pragma once

#include <stddef.h>
#include "Vector.h"

#define VectorTPart size_t
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

typedef void* VoidPtr;
#define VectorTPart VoidPtr
#include "Vector_Decl.h"
