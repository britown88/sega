#include "RTTI.h"

size_t getUniqueRTTIID(){
   static size_t _eCount = 0;
   return _eCount++;
}
