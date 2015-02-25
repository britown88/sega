#pragma once

#include "Strings.h"
#include "segautils\Preprocessor.h"
#include "segautils\DLLBullshit.h"

#include <stddef.h>

typedef struct {
   size_t ID;
   StringView name;
} Type;

DLL_PUBLIC size_t getUniqueRTTIID();

#define GetRTTI(Typename) CONCAT(_getRTTI_, Typename)()

#define DeclRTTI(Typename) \
   Type *GetRTTI(Typename);

#define ImplRTTI(Typename)\
   Type *GetRTTI(Typename) {\
      static Type out = {0};\
      static int init = 0;\
      if(!init) {\
         out.ID = getUniqueRTTIID();\
         out.name = stringIntern(STRINGIFY(Typename));\
         init = 1; \
      }\
      return &out;\
   }

