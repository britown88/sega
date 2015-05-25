#pragma once
#include <string.h>

typedef struct String_t String;

//String *foo = stringCreate("test");
//printf((char*)foo);
//stringDestroy(foo);

String *stringCreate(const char *str);
void stringDestroy(String *self);
size_t stringLen(String *self);
void stringClear(String *self);
void stringConcat(String *self, const char*str);
const char *c_str(String *str);