#pragma once
#include <string.h>

#include "segautils/Defs.h"
#include "Vector.h"

typedef struct String_t String;

//String *foo = stringCreate("test");
//printf((char*)foo);
//stringDestroy(foo);

static const size_t stringNPos = (-1);

String *stringCreate(const char *str);
String *stringCopy(String *other);
void stringDestroy(String *self);
size_t stringLen(String *self);
size_t stringFindLastOf(String *self, const char *characters);
void stringClear(String *self);
void stringSubStr(String *self, size_t start, size_t len);
String *stringGetFilename(String *str); //returns new, truncates after last '.' and before last '/'
String *stringGetDirectory(String *str); //returns new, truncates after last '/'
void stringConcat(String *self, const char*str);
void stringConcatChar(String *self, const char c);
void stringConcatEX(String *self, const char*str, size_t length);
void stringSet(String *self, const char*str);
void stringInsert(String *self, char c, size_t pos);
void stringErase(String *self, size_t pos);
bool stringEqual(String *s1, String *s2);
bool stringEqualRaw(String *s1, const char *s2);
bool stringStartsWith(const char *s1, const char *s2, bool caseSensitive);
const char *c_str(String *str);


typedef String* StringPtr;
void stringPtrDestroy(StringPtr *self);
#define VectorTPart StringPtr
#include "Vector_Decl.h"

vec(StringPtr) *stringSplit(const char *self, char delim);

bool stringPtrCompare(StringPtr *str1, StringPtr *str2);
