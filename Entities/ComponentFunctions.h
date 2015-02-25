#include "segautils\Preprocessor.h"

#ifndef compList
#define compList(TYPE) CONCAT(compRegister_, TYPE)
#define compGetVTable(TYPE) CONCAT(compGetVTable_, TYPE)

#define entityAdd(TYPE) CONCAT(entityAdd_, TYPE)
#define entityGet(TYPE) CONCAT(entityGet_, TYPE)
#define entityRemove(TYPE) CONCAT(entityRemove_, TYPE)

#define compListCreate(TYPE) CONCAT(compListCreate_, TYPE)
#define compListDestroy(TYPE) CONCAT(compListDestroy_, TYPE)
#define compListGetAt(TYPE) CONCAT(compListGetAt_, TYPE)
#define compListCount(TYPE) CONCAT(compListCount_, TYPE)
#define compListAddComp(TYPE) CONCAT(compListAddComp_, TYPE)
#define compListRemoveComp(TYPE) CONCAT(compListRemoveComp_, TYPE)
#define compListGetRaw(TYPE) CONCAT(compListGetRaw_, TYPE)

#endif