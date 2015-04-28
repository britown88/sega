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

#define CreateRTTI(TypeName)\
   DeclRTTI(TypeName)\
   ImplRTTI(TypeName)

#define getUniqueLocalRTTIID(TAG) CONCAT(getUniqueLocalRTTIID_, TAG)()

#define DeclLocalRTTITag(TAG) \
   size_t getUniqueLocalRTTIID(TAG);

#define ImplLocalRTTITag(TAG) \
   size_t getUniqueLocalRTTIID(TAG){ \
      static size_t _eCount = 0; \
      return _eCount++; \
   }

#define CreateLocalRTTITag(TAG) \
   DeclLocalRTTITag(TAG) \
   ImplLocalRTTITag(TAG)

#define GetLocalRTTI(TAG, Typename) CONCAT(CONCAT(_getLocalRTTI_, TAG), Typename)()

#define DeclLocalRTTI(TAG, Typename) \
   Type *GetLocalRTTI(TAG, Typename);

#define ImplLocalRTTI(TAG, Typename)\
   Type *GetLocalRTTI(TAG, Typename) {\
      static Type out = {0};\
      static int init = 0;\
      if(!init) {\
         out.ID = getUniqueLocalRTTIID(TAG);\
         out.name = stringIntern(STRINGIFY(Typename));\
         init = 1; \
                  }\
      return &out;\
   }

#define CreateLocalRTTI(TAG, Typename) \
   DeclLocalRTTI(TAG, Typename) \
   ImplLocalRTTI(TAG, Typename)
