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
String *stringCopy(String *other) {
   return stringCreate(c_str(other));
}
void stringDestroy(String *self){
   vecDestroy(char)((vec(char)*)self);
}
size_t stringLen(String *self){
   return vecSize(char)((vec(char)*)self) - 1;
}
void stringClear(String *self){
   static char close = 0;
   vecClear(char)((vec(char)*)self);
   vecPushBack(char)((vec(char)*)self, &close);
}
void stringConcat(String *self, const char*str){
   stringConcatEX(self, str, strlen(str));
}
void stringConcatEX(String *self, const char*str, size_t length) {
   static char close = 0;
   vecPopBack(char)((vec(char)*)self);//kill the terminator
   vecPushArray(char)((vec(char)*)self, (char*)str, length);
   vecPushBack(char)((vec(char)*)self, &close);
}
void stringSet(String *self, const char*str) {
   static char close = 0;
   vecClear(char)((vec(char)*)self);
   vecPushArray(char)((vec(char)*)self, (char*)str, strlen(str));
   vecPushBack(char)((vec(char)*)self, &close);
}

const char *c_str(String *str){
   if (!str) {
      return NULL;
   }
   return vecAt(char)((vec(char)*)str, 0);
}