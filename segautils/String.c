#include "String.h"
#include "StandardVectors.h"
#include "segashared\CheckedMemory.h"

struct String_t{
   vec(char) *str;
};

String *stringCreate(const char *str){
   String *out = checkedCalloc(1, sizeof(String));   

   out->str = vecCreate(char)(NULL);
   stringConcat(out, str);

   return out;
}
void stringDestroy(String *self){
   vecDestroy(char)(self->str);
   checkedFree(self);
}
size_t stringLen(String *self){
   return vecSize(char)(self->str);
}
void stringClear(String *self){
   vecClear(char)(self->str);
}
void stringConcat(String *self, const char*str){
   static char close = 0;
   vecPopBack(char)(self->str);//kill the terminator
   vecPushArray(char)(self->str, (char*)str, strlen(str));
   vecPushBack(char)(self->str, &close);
}

const char *c_str(String *str){
   return vecAt(char)(str->str, 0);
}