#pragma once
#include <string.h>

typedef struct String_t String;

//String *foo = stringCreate("test");
//printf((char*)foo);
//stringDestroy(foo);

String *stringCreate(const char *str);
String *stringCopy(String *other);
void stringDestroy(String *self);
size_t stringLen(String *self);
void stringClear(String *self);
void stringSubStr(String *self, size_t start, size_t len);
void stringConcat(String *self, const char*str);
void stringConcatEX(String *self, const char*str, size_t length);
void stringSet(String *self, const char*str);
void stringInsert(String *self, char c, size_t pos);
void stringErase(String *self, size_t pos);
const char *c_str(String *str);