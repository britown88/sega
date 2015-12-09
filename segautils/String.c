#include "String.h"
#include "StandardVectors.h"
#include "segashared\CheckedMemory.h"
#include "Defs.h"

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
void stringSubStr(String *self, size_t start, size_t len) {
   static char close = 0;
   size_t actuallen = stringLen(self);
   size_t finalLen = MIN(len - start, actuallen - start);

   if (start >= actuallen) {
      return;
   }

   memcpy((char*)c_str(self), (char*)c_str(self) + start, finalLen);
   vecResize(char)((vec(char)*)self, finalLen, &(char){0});
   vecPushBack(char)((vec(char)*)self, &(char){0});
}
void stringClear(String *self){
   vecClear(char)((vec(char)*)self);
   vecPushBack(char)((vec(char)*)self, &(char){0});
}
void stringConcat(String *self, const char*str){
   stringConcatEX(self, str, strlen(str));
}
void stringConcatEX(String *self, const char*str, size_t length) {
   vecPopBack(char)((vec(char)*)self);//kill the terminator
   vecPushArray(char)((vec(char)*)self, (char*)str, length);
   vecPushBack(char)((vec(char)*)self, &(char){0});
}
void stringSet(String *self, const char*str) {
   vecClear(char)((vec(char)*)self);
   vecPushArray(char)((vec(char)*)self, (char*)str, strlen(str));
   vecPushBack(char)((vec(char)*)self, &(char){0});
}

const char *c_str(String *str){
   if (!str) {
      return NULL;
   }
   return vecAt(char)((vec(char)*)str, 0);
}

void stringInsert(String *self, char c, size_t pos) {
   if (pos <= stringLen(self)) {      
      vecInsert(char)((vec(char)*)self, pos, &c);
   }
}
void stringErase(String *self, size_t pos) {
   if (pos < stringLen(self)) {
      vecRemoveAt(char)((vec(char)*)self, pos);
   }
}

bool stringEqual(String *s1, String *s2) {
   size_t len1 = stringLen(s1);
   size_t len2 = stringLen(s2);

   if (len1 != len2) {
      return false;
   }

   return memcmp(c_str(s1), c_str(s2), len1) == 0;   
}