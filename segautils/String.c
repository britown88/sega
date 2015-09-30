#include "String.h"
#include "StandardVectors.h"
#include "segashared\CheckedMemory.h"

#pragma pack(push, 1)
struct String_t{
   vec(char) *str;
};
#pragma pack(pop)

String *stringCreate(const char *str){
   vec(char) *out = vecCreate(char)(NULL);

   stringConcat((String*)out, str);

   return (String*)out;
}
void stringDestroy(String *self){
   vecDestroy(char)((vec(char)*)self);
}
size_t stringLen(String *self){
   return vecSize(char)((vec(char)*)self);
}
void stringClear(String *self){
   vecClear(char)((vec(char)*)self);
}
void stringConcat(String *self, const char*str){
   static char close = 0;
   vecPopBack(char)((vec(char)*)self);//kill the terminator
   vecPushArray(char)((vec(char)*)self, (char*)str, strlen(str));
   vecPushBack(char)((vec(char)*)self, &close);
}

const char *c_str(String *str){
   if (!str) {
      return NULL;
   }
   return vecAt(char)((vec(char)*)str, 0);
}