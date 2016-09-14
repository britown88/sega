/*
- an add table call for sql
- insert/select/remove lua calls
- struct in C
- insert/select/remove calls in C
DBInsert(Actor)(view->db, &(Actor){.name = "blah", .location = "westerfordshire"});
*/

//struct member
//{
//   std::vector<modifer> modifiers; //maybe a bitfield? PRIMARY_KEY, AUTO_INCREMENT, etc, might need more data
//   e_Type type;  //maybe a string?
//   std::string name;
//};
//
//struct parsed_struct
//{
//   std::vector<member> members;
//};

/*struct member
{
   std::vector<modifer> modifiers; //maybe a bitfield? PRIMARY_KEY, AUTO_INCREMENT, etc, might need more data
   e_Type type;  //maybe a string?
   std::string name;
};

struct parsed_struct
{
   std::vector<member> members;
};*/

//gramma
/*

::=         ::= "Is defined by"
()          ::= Optional
[]          ::= 0-n
|           ::= OR


//Tokenizer

FILE        ::= [SKIPPABLE|TOKEN]
SKIPPABLE   ::= [COMMENT|WHITESPACE]
TOKEN       ::= GRAMMAR | IDENTIFIER
GRAMMAR     ::= '{' | '}' | '(' | ')' | ';'
IDENTIFIER  ::= IDENT_CHAR[IDENT_CHAR|NUMBER]
CHAR        ::= a-z A-Z
NUMBER      ::= 0-9
IDENT_CHAR  ::= CHAR | '_'
COMMENT     ::= C_COMMENT|CPP_COMMENT
C_COMMENT   ::= "we know what this is"
CPP_COMMENT ::= read until \n or EOF
WHITESPACE  ::= space tab or newline

*/

#include "segautils/Defs.h"
#include "segashared/CheckedMemory.h"
#include "segautils/String.h"
#include "segashared/Strings.h"
#include "segautils/BitBuffer.h"
#include "segautils/StandardVectors.h"
#include "SEGA/App.h"

typedef struct {
   char *pos, *last;
}StringStream;

bool sstrmAtEnd(StringStream *self) {
   return self->pos >= self->last;
}

char sstrmPeek(StringStream *self) {
   return *self->pos;
}

void sstrmSkip(StringStream *self, int skip) {
   self->pos += skip;
}

void sstrmRewind(StringStream *self, int skip) {
   self->pos -= skip;
}

void sstrmNext(StringStream *self) {
   ++self->pos;
}

char sstrmPop(StringStream *self) {
   return *self->pos++;
}

bool sstrmAccept(StringStream *self, char c) {
   return *self->pos == c ? !!(++self->pos) : false;
}

bool sstrmAcceptString(StringStream *self, const char *str) {
   const char *strStart = str;
   while (*str) {
      if (!sstrmAccept(self, *str)) {
         sstrmRewind(self, str - strStart);
         return false;
      }
      ++str;
   }
   return true;
}

typedef enum {
   TOKEN_IDENTIFIER = 0,
   TOKEN_OPERATOR,//curly braces n shit
   TOKEN_COUNT
}TokenType;

typedef struct {
   TokenType type;
   union {
      char operator;
      StringView identifier;
   };
}Token;

#define VectorT Token
#include "segautils/Vector_Create.h"

typedef struct {
   StringStream strm;
   vec(Token) *tokens;
}Tokenizer;

Tokenizer *tokenizerCreate(StringStream strm) {
   Tokenizer *out = checkedCalloc(1, sizeof(Tokenizer));
   out->strm = strm;
   out->tokens = vecCreate(Token)(NULL);
   return out;
}

void tokenizerDestroy(Tokenizer *self) {
   vecDestroy(Token)(self->tokens);
   checkedFree(self);
}


#define STRM &self->strm

bool tokenizerAcceptChar(Tokenizer *self) {
   char c = sstrmPeek(STRM);

   if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      sstrmPop(STRM);
      return true;
   }

   return false;
}
bool tokenizerAcceptNumber(Tokenizer *self) {
   char c = sstrmPeek(STRM);

   if ((c >= '0' && c <= '9')) {
      sstrmPop(STRM);
      return true;
   }

   return false;
}
bool tokenizerAcceptIdentChar(Tokenizer *self) {
   return tokenizerAcceptChar(self) || sstrmAccept(STRM, '_');
}
bool tokenizerAcceptIdentifier(Tokenizer *self) {   
   String *id = NULL;
   if (!tokenizerAcceptIdentChar(self)) {
      return false;
   }

   id = stringCreate("");
   sstrmRewind(STRM, 1);

   while (!sstrmAtEnd(STRM)) {
      char c = sstrmPeek(STRM);

      if (!tokenizerAcceptIdentChar(self) && !tokenizerAcceptNumber(self)) {
         break;
      }

      stringConcatChar(id, c);
   }

   vecPushBack(Token)(self->tokens, &(Token){.type = TOKEN_IDENTIFIER, .identifier = stringIntern(c_str(id)) });
   stringDestroy(id);

   return true;
}
bool tokenizerAcceptGrammar(Tokenizer *self) {
   static char chars[] = { '{', '}', '(', ')', ';' };
   int i;
   for (i = 0; i < sizeof(chars); ++i) {

      if (sstrmAccept(STRM, chars[i])) {
         vecPushBack(Token)(self->tokens, &(Token){.type = TOKEN_OPERATOR, .operator = chars[i] });
         return true;
      }
   }
   return false;
}
bool tokenizerAcceptWhitespace(Tokenizer *self) {
   static char chars[] = {' ', '\t', '\n', '\r' };
   int i;
   for (i = 0; i < sizeof(chars); ++i) {
      if (sstrmAccept(STRM, chars[i])) {
         return true;
      }
   }
   return false;
}
bool tokenizerAcceptCPPComment(Tokenizer *self) {
   if (!sstrmAcceptString(STRM, "//")) {
      return false;
   }

   while (!sstrmAtEnd(STRM)) {
      char c = sstrmPop(STRM);
      if (c == '\n') {
         break;
      }
   }

   return true;
}
bool tokenizerAcceptCComment(Tokenizer *self) {
   if (!sstrmAcceptString(STRM, "/*")) {
      return false;
   }

   while (!sstrmAtEnd(STRM)) {
      if (sstrmAcceptString(STRM, "*/")) {
         break;
      }
      sstrmNext(STRM);
   }

   return true;
}
bool tokenizerAcceptComment(Tokenizer *self) {
   return tokenizerAcceptCComment(self) || tokenizerAcceptCPPComment(self);
}
bool tokenizerAcceptToken(Tokenizer *self) {
   return tokenizerAcceptGrammar(self) || tokenizerAcceptIdentifier(self);
}
bool tokenizerAcceptSkippable(Tokenizer *self) {
   bool hit = false;
   while (tokenizerAcceptComment(self) || tokenizerAcceptWhitespace(self)) { hit = true; }
   return hit;
}
void tokenizerAcceptFile(Tokenizer *self) {
   while (!sstrmAtEnd(STRM)) {
      if (tokenizerAcceptSkippable(self)) {
         continue;
      }

      tokenizerAcceptToken(self);
   }
}

//now tot ake those tokens and turn them into shit
/*

FILE        ::= [STRUCT]
STRUCT      ::= "struct" IDENTIFIER OP{ [MEMBER] OP} OP;
MEMBER      ::= [MODIFIER] TYPE IDENTIFIER OP;
MODIFIER    ::= "PRIMARY_KEY" | "UNIQUE" | "REPLACE_ON_CONFLICT" | "AUTOINCREMENT"
TYPE        ::= "int" | "String" | "float" | "bool" | "Blob" | "char"

*/

typedef enum {
   MODIFIER_PRIMARY_KEY = 0,
   MODIFIER_UNIQUE,
   MODIFIER_REPLACE_ON_CONFLICT,
   MODIFIER_AUTOINCREMENT,
   MODIFIER_SELECT,
   MODIFIER_COUNT
}Modifier;

#define MOD(m) (1 << m)

static StringView mods[MODIFIER_COUNT] = { 0 };

typedef struct {
   Token *pos, *last;
}TokenStream;

bool tstrmAtEnd(TokenStream *self) {
   return self->pos >= self->last;
}
Token tstrmPeek(TokenStream *self) {
   return *self->pos;
}
void tstrmSkip(TokenStream *self, int skip) {
   self->pos += skip;
}
void tstrmRewind(TokenStream *self, int skip) {
   self->pos -= skip;
}
void tstrmNext(TokenStream *self) {
   ++self->pos;
}
Token tstrmPop(TokenStream *self) {
   return *self->pos++;
}
bool tstrmAcceptIdentifier(TokenStream *self, StringView id) {
   Token t = tstrmPeek(self);
   if (t.type != TOKEN_IDENTIFIER) {
      return false;
   }
   else if(t.identifier == id){
      tstrmNext(self);
      return true;
   }

   return false;
}
StringView tstrmAcceptIdentifierAny(TokenStream *self) {
   Token t = tstrmPeek(self);
   if (t.type != TOKEN_IDENTIFIER) {
      return NULL;
   }
   else{
      tstrmNext(self);
      return t.identifier;
   }
}
bool tstrmAcceptOperator(TokenStream *self, char c) {
   Token t = tstrmPeek(self);
   if (t.type != TOKEN_OPERATOR) {
      return false;
   }
   else if (t.operator == c) {
      tstrmNext(self);
      return true;
   }

   return false;
}

typedef enum {
   TYPE_INT = 0,
   TYPE_STRING,
   TYPE_FLOAT,
   TYPE_BOOL,
   TYPE_BLOB,
   TYPE_CHAR,
   TYPE_COUNT
}MemberType;

static StringView types[TYPE_COUNT] = { 0 };
static StringView CTypes[TYPE_COUNT] = { 0 };
static StringView SQLTypes[TYPE_COUNT] = { 0 };

typedef struct{
   int mods;//bitfield
   MemberType type;
   StringView name;
}DBMember;

#define VectorT DBMember
#include "segautils/Vector_Create.h"

typedef struct  {
   StringView name;
   vec(DBMember) *members;
}DBStruct;

void DBStructDestroy(DBStruct *self) {
   vecDestroy(DBMember)(self->members);
}

#define VectorT DBStruct
#include "segautils/Vector_Create.h"

typedef struct {
   TokenStream strm;
   vec(DBStruct) *structs;
}Lexer;

Lexer *lexerCreate(TokenStream strm) {
   Lexer *out = checkedCalloc(1, sizeof(Lexer));
   out->strm = strm;
   out->structs = vecCreate(DBStruct)(&DBStructDestroy);
   return out;
}

void lexerDestroy(Lexer *self) {
   vecDestroy(DBStruct)(self->structs);
   checkedFree(self);
}


MemberType lexerAcceptType(Lexer *self) {
   
   int i = 0;
   if (!types[0]) {
      types[i++] = stringIntern("int");
      types[i++] = stringIntern("String");
      types[i++] = stringIntern("float");
      types[i++] = stringIntern("bool");
      types[i++] = stringIntern("Blob");
      types[i++] = stringIntern("char");
   }

   for (i = 0; i < TYPE_COUNT; ++i) {
      if (tstrmAcceptIdentifier(STRM, types[i])) {
         return i;
      }
   }

   return TYPE_COUNT;
}
Modifier lexerAcceptModifier(Lexer *self) {
   
   int i = 0;
   if (!mods[0]) {
      mods[i++] = stringIntern("PRIMARY_KEY");
      mods[i++] = stringIntern("UNIQUE");
      mods[i++] = stringIntern("REPLACE_ON_CONFLICT");
      mods[i++] = stringIntern("AUTOINCREMENT");
      mods[i++] = stringIntern("SELECT");
   }

   for (i = 0; i < MODIFIER_COUNT; ++i) {
      if (tstrmAcceptIdentifier(STRM, mods[i])) {
         return i;
      }
   }

   return MODIFIER_COUNT;
}
DBMember lexerAcceptMember(Lexer *self) {
   static DBMember NullOut = { .name = NULL };
   Modifier newMod;

   DBMember newMember = { 0 };
   

   while ((newMod = lexerAcceptModifier(self)) != MODIFIER_COUNT) {
      newMember.mods |= MOD(newMod);// :D
   }

   newMember.type = lexerAcceptType(self);

   if (newMember.type == TYPE_COUNT) {
      return NullOut;
   }

   newMember.name = tstrmAcceptIdentifierAny(STRM);

   if (!newMember.name) {
      return NullOut;
   }

   if (!tstrmAcceptOperator(STRM, ';')) {
      return NullOut;
   }

   return newMember;
}
DBStruct lexerAcceptStruct(Lexer *self) {
   static DBStruct NullOut = { .name = NULL };
   static StringView svStruct = NULL;
   DBMember newMember = { 0 };
   DBStruct newStruct = { 0 };

   if (!svStruct) {
      svStruct = stringIntern("struct");
   }

   if (!tstrmAcceptIdentifier(STRM, svStruct)) { 
      return NullOut;
   }

   newStruct.name = tstrmAcceptIdentifierAny(STRM);

   if (!newStruct.name) {
      return NullOut;
   }

   if (!tstrmAcceptOperator(STRM, '{')) {
      return NullOut;
   }

   newStruct.members = vecCreate(DBMember)(NULL);
   while ((newMember = lexerAcceptMember(self)).name) {
      vecPushBack(DBMember)(newStruct.members, &newMember);
   }

   if (!tstrmAcceptOperator(STRM, '}')) {
      vecDestroy(DBMember)(newStruct.members);
      return NullOut;
   }

   if (!tstrmAcceptOperator(STRM, ';')) {
      vecDestroy(DBMember)(newStruct.members);
      return NullOut;
   }

   return newStruct;

}
void lexerAcceptFile(Lexer *self) {
   DBStruct newStruct;
   while ((newStruct = lexerAcceptStruct(self)).name) {
      vecPushBack(DBStruct)(self->structs, &newStruct);
   }
}

#undef STRM

#include <stdio.h>

static void _printTestOutput(byte *buff, Tokenizer *tokens, Lexer *lexer) {
   printf("FILE:\n-----\n%s\n", buff);

   printf("TOKENS:\n-------\n");
   vecForEach(Token, t, tokens->tokens, {

      if (t->type == TOKEN_IDENTIFIER) {
         printf("ID %s\n", t->identifier);
      }
      else {
         printf("OP %c\n", t->operator);
      }
   });

   printf("\nDBStructs:\n----------\n");

   vecForEach(DBStruct, s, lexer->structs, {
      printf("Struct '%s' with %i members:\n", s->name, vecSize(DBMember)(s->members));
      vecForEach(DBMember, member, s->members,{
         int modCount = 0;
         int i;

         printf("   Type: %i (%s)\n   Name: '%s'\n", member->type, types[member->type], member->name);

         for (i = 0; i < MODIFIER_COUNT; ++i) {
            if (member->mods&(1 << i)) { ++modCount; }
         }

         printf("   %i Modifiers%c\n", modCount, modCount > 0 ? ':' : ' ');

         for (i = 0; i < MODIFIER_COUNT; ++i) {
            if (member->mods&(1 << i)) {
               printf("      %i (%s)\n", i, mods[i]);
            }
         }

         printf("\n");
      });
   });

   scanf("");
}

typedef struct {
   String *dir, *input, *inputFileOnly, *outputh, *outputc;
   vec(DBStruct) *structs;
}FileData;

void headerWriteHeader(FILE *f, FileData *data) {
   fprintf(f, 
      "/***********************************************************************\n"
      "   WARNING: This file generated by robots.  Do not attempt to modify.\n\n"
      "   This API is for use with %s.db\n"
      "   Which contains %i table(s).\n"
      "***********************************************************************/\n\n"
      "#pragma once\n\n"
      "#include \"segautils/Defs.h\"\n"
      "#include \"segautils/String.h\"\n\n"
      "typedef struct sqlite3_stmt sqlite3_stmt;\n"
      "typedef struct lua_State lua_State;\n"      
      "typedef struct DB_%s DB_%s;\n"
      
      "\n", 
         c_str(data->inputFileOnly), //.db
         vecSize(DBStruct)(data->structs), //x tables
      c_str(data->inputFileOnly), c_str(data->inputFileOnly)
      );
}
void headerCreateStruct(FILE *f, FileData *fd, DBStruct *strct) {
   int i = 0;

   if (!CTypes[0]) {
      CTypes[i++] = stringIntern("int ");
      CTypes[i++] = stringIntern("const char *");
      CTypes[i++] = stringIntern("float ");
      CTypes[i++] = stringIntern("bool ");
      CTypes[i++] = stringIntern("void *");
      CTypes[i++] = stringIntern("char ");
   }

   //the struct
   fprintf(f, "typedef struct {\n");
   vecForEach(DBMember, mem, strct->members, {
      switch (mem->type) {
      case TYPE_BLOB: fprintf(f,    "   void *%s;\n   int %sSize;\n", mem->name, mem->name); break;
      case TYPE_INT: fprintf(f,     "   int %s;\n", mem->name); break;
      case TYPE_STRING: fprintf(f,  "   String *%s;\n", mem->name); break;
      case TYPE_BOOL: fprintf(f,    "   bool %s;\n", mem->name); break;
      case TYPE_CHAR: fprintf(f,    "   char %s;\n", mem->name); break;
      case TYPE_FLOAT: fprintf(f,   "   float %s;\n", mem->name); break;
      }
   });
   fprintf(f, "} DB%s;\n\n", strct->name);

   //vector
   fprintf(f, "#define VectorTPart DB%s\n#include \"segautils/Vector_Decl.h\"\n\n", strct->name);

   //functions
   fprintf(f, "void db%sDestroy(DB%s *self); //this does not call free on self!!\n", strct->name, strct->name);   
   fprintf(f, "int db%sInsert(DB_%s *db, const DB%s *obj);\n", strct->name, c_str(fd->inputFileOnly), strct->name);
   fprintf(f, "int db%sUpdate(DB_%s *db, const DB%s *obj); //will base on primary key\n", strct->name, c_str(fd->inputFileOnly), strct->name);
   fprintf(f, "vec(DB%s) *db%sSelectAll(DB_%s *db);\n", strct->name, strct->name, c_str(fd->inputFileOnly));

   vecForEach(DBMember, member, strct->members, {
      if (member->mods&MOD(MODIFIER_SELECT)) { 

         fprintf(f, "DB%s db%sSelectFirstBy%s(DB_%s *db, %s%s);\n",
            strct->name, strct->name, member->name,
            c_str(fd->inputFileOnly), CTypes[member->type], member->name);

         if (!(member->mods&MOD(MODIFIER_UNIQUE))) {
            fprintf(f, "vec(DB%s) *db%sSelectBy%s(DB_%s *db, %s%s);\n",
               strct->name, strct->name, member->name,
               c_str(fd->inputFileOnly), CTypes[member->type], member->name);
         }
      }
   });

   fprintf(f, "int db%sDeleteAll(DB_%s *db);\n", strct->name, c_str(fd->inputFileOnly));

   vecForEach(DBMember, member, strct->members, {
      if (member->mods&MOD(MODIFIER_SELECT)) {

         fprintf(f, "int db%sDeleteBy%s(DB_%s *db, %s%s);\n",
            strct->name, member->name,
            c_str(fd->inputFileOnly), CTypes[member->type], member->name);

      }
   });

   fprintf(f, "\n");
}

void headerCreateDB(FILE *f, FileData *fd) {
   fprintf(f, "DB_%s *db_%sCreate();\n", c_str(fd->inputFileOnly), c_str(fd->inputFileOnly));
   fprintf(f, "void db_%sDestroy(DB_%s *self);\n", c_str(fd->inputFileOnly), c_str(fd->inputFileOnly));
   fprintf(f, "int db_%sCreateTables(DB_%s *self);\n\n", c_str(fd->inputFileOnly), c_str(fd->inputFileOnly));
}

void createHeader(FileData *data) {
   FILE *f = fopen(c_str(data->outputh), "w");

   if (!f) {
      printf("ERROR: Unable to create file %s.\n", c_str(data->outputh));
      return;
   }

   headerWriteHeader(f, data);

   headerCreateDB(f, data);

   vecForEach(DBStruct, strct, data->structs, {
      headerCreateStruct(f, data, strct);
   });

   

   fclose(f);
}

void sourceWriteHeader(FILE *f, FileData *data) {
   fprintf(f,
      "/***********************************************************************\n"
      "   WARNING: This file generated by robots.  Do not attempt to modify.\n\n"
      "   This API is for use with %s.db\n"
      "   Which contains %i table(s).\n"
      "***********************************************************************/\n\n"
      "#include \"%s\"\n"
      "#include \"DB.h\"\n"
      "#include \"segashared/CheckedMemory.h\"\n"

      "#include \"sqlite/sqlite3.h\"\n\n"

      , c_str(data->inputFileOnly), vecSize(DBStruct)(data->structs), c_str(data->outputh)

      );
}

void sourceWriteDestroy(FILE *f, FileData *fd, DBStruct *strct) {
   fprintf(f, "void db%sDestroy(DB%s *self){\n", strct->name, strct->name);

   vecForEach(DBMember, member, strct->members, {
      if (member->type == TYPE_STRING) {
         fprintf(f,
            "   if(self->%s){\n"
            "      stringDestroy(self->%s);\n"
            "      self->%s = NULL;\n"
            "   }\n", member->name, member->name, member->name);
      }
      else if (member->type == TYPE_BLOB) {
         fprintf(f,
            "   if(self->%s){\n"
            "      checkedFree(self->%s);\n"
            "      self->%s = NULL;\n"
            "   }\n", member->name, member->name, member->name);
      }
   });

   fprintf(f, "}\n");
}


void sourceWriteBindMember(FILE *f, FileData *fd, DBStruct *strct, DBMember *member, const char *stmt, size_t index) {

   switch (member->type) {
   case TYPE_INT:
   case TYPE_BOOL:
   case TYPE_CHAR:
      fprintf(f, "   result = sqlite3_bind_int(db->%sStmts.%s, %i, (int)obj->%s);\n",
                     strct->name, stmt, index, member->name);
      break;
   case TYPE_BLOB:
      fprintf(f, "   result = sqlite3_bind_blob(db->%sStmts.%s, %i, obj->%s, obj->%sSize, SQLITE_TRANSIENT);\n",
                     strct->name, stmt, index, member->name, member->name);
      break;
   case TYPE_FLOAT:
      fprintf(f, "   result = sqlite3_bind_double(db->%sStmts.%s, %i, (double)obj->%s);\n",
                     strct->name, stmt, index, member->name);
      break;
   case TYPE_STRING:
      fprintf(f, "   result = sqlite3_bind_text(db->%sStmts.%s, %i, c_str(obj->%s), -1, NULL);\n",
                     strct->name, stmt, index, member->name);
      break;
   }

   fprintf(f,
      "   if (result != SQLITE_OK) {\n"
      "      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));\n"
      "      return DB_FAILURE;\n"
      "   }\n\n");
}

void sourceWriteBindMemberSelect(FILE *f, FileData *fd, DBStruct *strct, DBMember *member, const char *stmt, size_t index, const char *outName) {

   switch (member->type) {
   case TYPE_INT:
   case TYPE_BOOL:
   case TYPE_CHAR:
      fprintf(f, "   result = sqlite3_bind_int(db->%sStmts.%s, %i, (int)%s);\n",
         strct->name, stmt, index, member->name);
      break;
   case TYPE_BLOB:
      fprintf(f, "   result = sqlite3_bind_blob(db->%sStmts.%s, %i, %s, %sSize, SQLITE_TRANSIENT);\n",
         strct->name, stmt, index, member->name, member->name);
      break;
   case TYPE_FLOAT:
      fprintf(f, "   result = sqlite3_bind_double(db->%sStmts.%s, %i, (double)%s);\n",
         strct->name, stmt, index, member->name);
      break;
   case TYPE_STRING:
      fprintf(f, "   result = sqlite3_bind_text(db->%sStmts.%s, %i, %s, -1, NULL);\n",
         strct->name, stmt, index, member->name);
      break;
   }

   fprintf(f,
      "   if (result != SQLITE_OK) {\n"
      "      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));\n"
      "      return %s;\n"
      "   }\n\n", outName);
}

void sourceWriteGetColumn(FILE *f, FileData *fd, DBStruct *strct, DBMember *member, const char *stmt, size_t index, const char *objName) {

   switch (member->type) {
   case TYPE_INT:
   case TYPE_BOOL:
      fprintf(f, "      %s.%s = sqlite3_column_int(db->%sStmts.%s, %i);\n",
         objName, member->name, strct->name, stmt, index);
      break;
   case TYPE_CHAR:
      fprintf(f, "      %s.%s = (char)sqlite3_column_int(db->%sStmts.%s, %i);\n",
         objName, member->name, strct->name, stmt, index);
      break;
   case TYPE_BLOB:
      fprintf(f, "      %s.%sSize = sqlite3_column_bytes(db->%sStmts.%s, %i);\n",
         objName, member->name, strct->name, stmt, index);

      fprintf(f,
         "      %s.%s = checkedCalloc(1, %s.%sSize);\n"
         "      memcpy(%s.%s, sqlite3_column_blob(db->%sStmts.%s, %i), %s.%sSize);\n",
         objName, member->name, objName, member->name, objName, member->name, strct->name, stmt, index, objName, member->name );
      

      break;
   case TYPE_FLOAT:
      fprintf(f, "      %s.%s = (float)sqlite3_column_double(db->%sStmts.%s, %i);\n",
         objName, member->name, strct->name, stmt, index);
      break;
   case TYPE_STRING:
      fprintf(f, "      %s.%s = stringCreate(sqlite3_column_text(db->%sStmts.%s, %i));\n",
         objName, member->name, strct->name, stmt, index);
      break;
   }



}

void sourceWriteStatementDestroy(FILE *f, const char *table, const char *stmt) {
   fprintf(f,
      "   if(db->%sStmts.%s){\n"
      "      sqlite3_finalize(db->%sStmts.%s);\n"
      "      db->%sStmts.%s = NULL;\n"
      "   }\n", table, stmt, table, stmt, table, stmt);
}
void sourceWriteDestroyStatements(FILE *f, FileData *fd, DBStruct *strct) {
   static char buff[256] = { 0 };
   fprintf(f, "void db%sDestroyStatements(DB_%s *db){\n", strct->name, c_str(fd->inputFileOnly));

   sourceWriteStatementDestroy(f, strct->name, "insert");
   sourceWriteStatementDestroy(f, strct->name, "update");
   sourceWriteStatementDestroy(f, strct->name, "selectAll");
   sourceWriteStatementDestroy(f, strct->name, "deleteAll");

   vecForEach(DBMember, member, strct->members, {
      if (member->mods&MOD(MODIFIER_SELECT)) {
         sprintf(buff, "selectBy%s", member->name);
         sourceWriteStatementDestroy(f, strct->name, buff);

         sprintf(buff, "deleteBy%s", member->name);
         sourceWriteStatementDestroy(f, strct->name, buff);
      }
   });

   fprintf(f, "}\n");
}
void sourceWriteCreateTable(FILE *f, FileData *fd, DBStruct *strct) {
   int i = 0;
   bool first = true;
   if (!SQLTypes[0]) {
      SQLTypes[i++] = stringIntern("INTEGER");
      SQLTypes[i++] = stringIntern("STRING");
      SQLTypes[i++] = stringIntern("REAL");
      SQLTypes[i++] = stringIntern("BOOLEAN");
      SQLTypes[i++] = stringIntern("BLOB");
      SQLTypes[i++] = stringIntern("CHAR");
   }

   fprintf(f, "int db%sCreateTable(DB_%s *db){\n", strct->name, c_str(fd->inputFileOnly));

   //the table creation string
   fprintf(f,
      "   static const char *cmd = \"CREATE TABLE \\\"%s\\\" (", strct->name);
   vecForEach(DBMember, member, strct->members, {
      if (!first) {
         fprintf(f, ", ");
      }
      else {
         first = false;
      }
      fprintf(f, "\\\"%s\\\"", member->name);
      fprintf(f, " %s", SQLTypes[member->type]);

      if (member->mods&MOD(MODIFIER_PRIMARY_KEY)) { fprintf(f, " PRIMARY KEY");  }
      if (member->mods&MOD(MODIFIER_AUTOINCREMENT)) { fprintf(f, " AUTOINCREMENT"); }
      if (member->mods&MOD(MODIFIER_UNIQUE)) { fprintf(f, " UNIQUE"); }
      if (member->mods&MOD(MODIFIER_REPLACE_ON_CONFLICT)) { fprintf(f, " ON CONFLICT REPLACE"); }

   });
   fprintf(f, ");\";\n");

   fprintf(f,
      "   return dbExecute((DBBase*)db, cmd);\n"
      );

   fprintf(f, "}\n");
}
void sourceWriteInsert(FILE *f, FileData *fd, DBStruct *strct) {
   bool first = true;
   int i = 0;
   fprintf(f, "int db%sInsert(DB_%s *db, const DB%s *obj){\n", strct->name, c_str(fd->inputFileOnly), strct->name);

   fprintf(f,
      "   int result = 0;\n"
      "   static const char *stmt = \"INSERT INTO \\\"%s\\\" ("
      , strct->name);

   vecForEach(DBMember, member, strct->members, {
      if (!(member->mods&MOD(MODIFIER_AUTOINCREMENT))) {
         if (!first) {
            fprintf(f, ", ");
         }
         else {
            first = false;
         }
         fprintf(f, "\\\"%s\\\"", member->name);
      }
   });

   fprintf(f, ") VALUES (");

   first = true;

   vecForEach(DBMember, member, strct->members, {
      if (!(member->mods&MOD(MODIFIER_AUTOINCREMENT))) {
         if (!first) {
            fprintf(f, ", ");
         }
         else {
            first = false;
         }
         fprintf(f, ":%s", member->name);
      }
   });

   fprintf(f, ");\";\n");
   fprintf(f,
      "   if(dbPrepareStatement((DBBase*)db, &db->%sStmts.insert, stmt) != DB_SUCCESS){\n"
      "      return DB_FAILURE;\n"
      "   }\n\n"
      "   //bind the values\n", strct->name
      );

   i = 1;
   vecForEach(DBMember, member, strct->members, {
      if (!(member->mods&MOD(MODIFIER_AUTOINCREMENT))) {
         sourceWriteBindMember(f, fd, strct, member, "insert", i++);
      }
   });

   //now run it
   fprintf(f,
      "   //now run it\n"
      "   result = sqlite3_step(db->%sStmts.insert);\n"
      "   if (result != SQLITE_DONE) {\n"
      "      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));\n"
      "      return DB_FAILURE;\n"
      "   }\n\n"
      "   return DB_SUCCESS;\n", strct->name
      );

   fprintf(f, "}\n");
}
void sourceWriteUpdate(FILE *f, FileData *fd, DBStruct *strct) {
   bool first = true;
   int i = 0;
   DBMember *pkey = NULL;
   fprintf(f, "int db%sUpdate(DB_%s *db, const DB%s *obj){\n", strct->name, c_str(fd->inputFileOnly), strct->name);

   fprintf(f,
      "   int result = 0;\n"
      "   static const char *stmt = \"UPDATE \\\"%s\\\" SET ("
      , strct->name);

   vecForEach(DBMember, member, strct->members, {
      if (member->mods&MOD(MODIFIER_PRIMARY_KEY)) {
         pkey = member;
         continue;
      }

      if (!(member->mods&MOD(MODIFIER_AUTOINCREMENT))) {         

         if (!first) {
            fprintf(f, ", ");
         }
         else {
            first = false;
         }
         fprintf(f, "\\\"%s\\\" = :%s", member->name, member->name);
      }
   });

   fprintf(f, ") WHERE (\\\"%s\\\" = :%s)\";\n", pkey->name, pkey->name);


   fprintf(f,
      "   if(dbPrepareStatement((DBBase*)db, &db->%sStmts.update, stmt) != DB_SUCCESS){\n"
      "      return DB_FAILURE;\n"
      "   }\n\n"
      "   //bind the values\n", strct->name
      );

   i = 1;
   vecForEach(DBMember, member, strct->members, {
      if (!(member->mods&MOD(MODIFIER_AUTOINCREMENT)) && !(member->mods&MOD(MODIFIER_PRIMARY_KEY))) {
         sourceWriteBindMember(f, fd, strct, member, "update", i++);
      }
   });

   //and the pkey
   fprintf(f, "   //primary key:\n");
   sourceWriteBindMember(f, fd, strct, pkey, "update", i++);

   //now run it
   fprintf(f,
      "   //now run it\n"
      "   result = sqlite3_step(db->%sStmts.update);\n"
      "   if (result != SQLITE_DONE) {\n"
      "      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));\n"
      "      return DB_FAILURE;\n"
      "   }\n\n"
      "   return DB_SUCCESS;\n", strct->name
      );

   fprintf(f, "}\n");
}
void sourceWriteSelectAll(FILE *f, FileData *fd, DBStruct *strct) {
   

   bool first = true;
   int i = 0;
   fprintf(f, "vec(DB%s) *db%sSelectAll(DB_%s *db){\n", strct->name, strct->name, c_str(fd->inputFileOnly));

   fprintf(f,
      "   int result = 0;\n"
      "   static const char *stmt = \"SELECT * FROM \\\"%s\\\";\";\n", strct->name);

   fprintf(f,
      "   if(dbPrepareStatement((DBBase*)db, &db->%sStmts.selectAll, stmt) != DB_SUCCESS){\n"
      "      return NULL;\n"
      "   }\n\n", strct->name
      );

   fprintf(f,
      "   vec(DB%s) *out = vecCreate(DB%s)(&db%sDestroy);\n\n"

      "   while((result = sqlite3_step(db->%sStmts.selectAll)) == SQLITE_ROW){\n"
      "      DB%s newObj = {0};\n\n"
      , strct->name, strct->name, strct->name, strct->name, strct->name
      );

   
   i = 0;
   vecForEach(DBMember, member, strct->members, {
      sourceWriteGetColumn(f, fd, strct, member, "selectAll", i++, "newObj");
   });

   fprintf(f,
      "      \n"
      "      vecPushBack(DB%s)(out, &newObj);\n\n"
      "   };\n\n"

      "   if(result != SQLITE_DONE){\n"
      "      vecDestroy(DB%s)(out);\n"
      "      return NULL;\n"
      "   }\n\n"
      "   return out;\n", strct->name, strct->name);

   fprintf(f, "}\n");
}
void sourceWriteSelectFirst(FILE *f, FileData *fd, DBStruct *strct, DBMember *member) {
   
   bool first = true;
   int i = 0;
   char stmtName[256] = { 0 };
   sprintf(stmtName, "selectBy%s", member->name);

   fprintf(f, "DB%s db%sSelectFirstBy%s(DB_%s *db, %s%s){\n",
      strct->name, strct->name, member->name,
      c_str(fd->inputFileOnly), CTypes[member->type], member->name);

   fprintf(f,
      "   DB%s out = {0};\n"
      "   int result = 0;\n"
      "   static const char *stmt = \"SELECT * FROM \\\"%s\\\" WHERE \\\"%s\\\" = :%s;\";\n", strct->name, strct->name, member->name, member->name);

   fprintf(f,
      "   if(dbPrepareStatement((DBBase*)db, &db->%sStmts.%s, stmt) != DB_SUCCESS){\n"
      "      return out;\n"
      "   }\n\n", strct->name, stmtName
      );

   sourceWriteBindMemberSelect(f, fd, strct, member, stmtName, 1, "out");

   fprintf(f,
      "   if((result = sqlite3_step(db->%sStmts.%s)) == SQLITE_ROW){\n"

      , strct->name, stmtName
      );


   i = 0;
   vecForEach(DBMember, member, strct->members, {
      sourceWriteGetColumn(f, fd, strct, member, stmtName, i++, "out");
   });

   fprintf(f,
      "   };\n\n"
      "   return out;\n");

   fprintf(f, "}\n");
}
void sourceWriteSelectBy(FILE *f, FileData *fd, DBStruct *strct, DBMember *member) {
   


   bool first = true;
   int i = 0;
   char stmtName[256] = { 0 };
   sprintf(stmtName, "selectBy%s", member->name);

   fprintf(f, "vec(DB%s) *db%sSelectBy%s(DB_%s *db, %s%s){\n",
      strct->name, strct->name, member->name,
      c_str(fd->inputFileOnly), CTypes[member->type], member->name);

   fprintf(f,
      "   int result = 0;\n"
      "   vec(DB%s) *out = NULL;\n"
      "   static const char *stmt = \"SELECT * FROM \\\"%s\\\" WHERE \\\"%s\\\" = :%s;\";\n"
      , strct->name, strct->name, member->name, member->name);

   fprintf(f,
      "   if(dbPrepareStatement((DBBase*)db, &db->%sStmts.%s, stmt) != DB_SUCCESS){\n"
      "      return NULL;\n"
      "   }\n\n", strct->name, stmtName
      );

   sourceWriteBindMemberSelect(f, fd, strct, member, stmtName, 1, "out");

   fprintf(f,
      "   out = vecCreate(DB%s)(&db%sDestroy);\n\n"

      "   while((result = sqlite3_step(db->%sStmts.%s)) == SQLITE_ROW){\n"
      "      DB%s newObj = {0};\n\n"
      , strct->name, strct->name, strct->name, stmtName, strct->name
      );


   i = 0;
   vecForEach(DBMember, member, strct->members, {
      sourceWriteGetColumn(f, fd, strct, member, stmtName, i++, "newObj");
   });

   fprintf(f,
      "      \n"
      "      vecPushBack(DB%s)(out, &newObj);\n\n"
      "   };\n\n"

      "   if(result != SQLITE_DONE){\n"
      "      vecDestroy(DB%s)(out);\n"
      "      return NULL;\n"
      "   }\n\n"
      "   return out;\n", strct->name, strct->name);

   fprintf(f, "}\n");
}
void sourceWriteDeleteAll(FILE *f, FileData *fd, DBStruct *strct) {
   fprintf(f, "int db%sDeleteAll(DB_%s *db){\nreturn DB_SUCCESS;\n}\n", strct->name, c_str(fd->inputFileOnly));
}
void sourceWriteDeleteBy(FILE *f, FileData *fd, DBStruct *strct, DBMember *member) {
   

   bool first = true;
   int i = 0;
   char stmtName[256] = { 0 };

   sprintf(stmtName, "deleteBy%s", member->name);

   fprintf(f, "int db%sDeleteBy%s(DB_%s *db, %s%s){\n",
      strct->name, member->name,
      c_str(fd->inputFileOnly), CTypes[member->type], member->name);

   fprintf(f,
      "   int result = 0;\n"
      "   static const char *stmt = \"DELETE FROM \\\"%s\\\" WHERE (\\\"%s\\\" = :%s);\";\n"
      , strct->name, member->name, member->name);

   fprintf(f,
      "   if(dbPrepareStatement((DBBase*)db, &db->%sStmts.%s, stmt) != DB_SUCCESS){\n"
      "      return DB_FAILURE;\n"
      "   }\n\n", strct->name, stmtName
      );


   //and the pkey
   fprintf(f, "   //primary key:\n");
   sourceWriteBindMemberSelect(f, fd, strct, member, stmtName, 1, "DB_FAILURE");

   //now run it
   fprintf(f,
      "   //now run it\n"
      "   result = sqlite3_step(db->%sStmts.%s);\n"
      "   if (result != SQLITE_DONE) {\n"
      "      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));\n"
      "      return DB_FAILURE;\n"
      "   }\n\n"
      "   return DB_SUCCESS;\n", strct->name, stmtName
      );

   fprintf(f, "}\n");
}


void sourceCreateStruct(FILE *f, FileData *fd, DBStruct *strct) {

   fprintf(f, "#define VectorTPart DB%s\n#include \"segautils/Vector_Impl.h\"\n\n", strct->name);

   sourceWriteDestroy(f, fd, strct);
   sourceWriteDestroyStatements(f, fd, strct);
   sourceWriteCreateTable(f, fd, strct);
   sourceWriteInsert(f, fd, strct);
   sourceWriteUpdate(f, fd, strct);
   sourceWriteSelectAll(f, fd, strct);   

   vecForEach(DBMember, member, strct->members, {
      if (member->mods&MOD(MODIFIER_SELECT)) {

         sourceWriteSelectFirst(f, fd, strct, member);

         if (!(member->mods&MOD(MODIFIER_UNIQUE))) {
            sourceWriteSelectBy(f, fd, strct, member);
         }
      }
   });

   sourceWriteDeleteAll(f, fd, strct);

   vecForEach(DBMember, member, strct->members, {
      if (member->mods&MOD(MODIFIER_SELECT)) {
         sourceWriteDeleteBy(f, fd, strct, member);
      }
   });
}

void sourceCreateDB(FILE *f, FileData *fd) {

   vecForEach(DBStruct, strct, fd->structs, {
      fprintf(f, "void db%sDestroyStatements(DB_%s *db);\n", strct->name, c_str(fd->inputFileOnly));
      fprintf(f, "int db%sCreateTable(DB_%s *db);\n", strct->name, c_str(fd->inputFileOnly));
   });

   fprintf(f, "\n");


   vecForEach(DBStruct, strct, fd->structs, {
      //statements struct
      fprintf(f, "typedef struct {\n");
      fprintf(f, "   sqlite3_stmt *insert;\n");
      fprintf(f, "   sqlite3_stmt *update;\n");
      fprintf(f, "   sqlite3_stmt *selectAll;\n");
      fprintf(f, "   sqlite3_stmt *deleteAll;\n");

      vecForEach(DBMember, member, strct->members,{
         if (member->mods&MOD(MODIFIER_SELECT)) {
            fprintf(f,
               "   sqlite3_stmt *selectBy%s;\n"
               "   sqlite3_stmt *deleteBy%s;\n",
               member->name, member->name);
         }
      });
      fprintf(f, "} DB%sStmts;\n\n", strct->name);
   });


   fprintf(f, "struct DB_%s{\n", c_str(fd->inputFileOnly));

   fprintf(f, "   DBBase base;\n");

   vecForEach(DBStruct, strct, fd->structs, {
      fprintf(f, "   DB%sStmts %sStmts;\n", strct->name, strct->name);
   });

   fprintf(f, "};\n\n");
   
   fprintf(f,
      "DB_%s *db_%sCreate(){\n"
      "   DB_%s *out = checkedCalloc(1, sizeof(DB_%s));\n"
      "   return out;\n"
      "}\n\n"
      , c_str(fd->inputFileOnly), c_str(fd->inputFileOnly), c_str(fd->inputFileOnly), c_str(fd->inputFileOnly));

   fprintf(f,
      "void db_%sDestroy(DB_%s *self){\n"
      "   dbDestroy((DBBase*)self);\n"      
      , c_str(fd->inputFileOnly), c_str(fd->inputFileOnly));

   vecForEach(DBStruct, strct, fd->structs, {
      fprintf(f, "   db%sDestroyStatements(self);\n", strct->name);
   });

   fprintf(f,
      "   checkedFree(self);\n"
      "}\n\n");

   fprintf(f,
      "int db_%sCreateTables(DB_%s *self){\n"
      "   int result = 0;\n\n", c_str(fd->inputFileOnly), c_str(fd->inputFileOnly));

   vecForEach(DBStruct, strct, fd->structs, {
      fprintf(f, "   if((result = db%sCreateTable(self)) != DB_SUCCESS){ return result; }\n", strct->name);
   });

   fprintf(f, "\n   return DB_SUCCESS;\n}\n\n");
}

void createSource(FileData *data) {
   FILE *f = fopen(c_str(data->outputc), "w");

   if (!f) {
      printf("ERROR: Unable to create file %s.\n", c_str(data->outputc));
      return;
   }

   sourceWriteHeader(f, data);

   sourceCreateDB(f, data);

   vecForEach(DBStruct, strct, data->structs, {
      sourceCreateStruct(f, data, strct);
   });

   

   fclose(f);
}

int main(int argc, char **argv) {
   byte *buff = NULL;
   long fSize = 0;
   Tokenizer *tokens = NULL;
   Lexer *lexer = NULL;
   FileData data = { 0 };
   char namebuff[256] = { 0 };

   printf("_________\n| DBGEN |\n");

   if (argc != 2) {
      printf("Usage: DBGen.exe [file]\n");
      return 0;
   }

   buff = readFullFile(argv[1], &fSize);

   if (!buff) {
      printf("File not found: %s\n", argv[1]);
      return 0;
   }

   printf("Input: %s\n", argv[1]);

   tokens = tokenizerCreate((StringStream) { .pos = buff, .last = buff + fSize });
   tokenizerAcceptFile(tokens);

   lexer = lexerCreate((TokenStream) { .pos = vecBegin(Token)(tokens->tokens), .last = vecEnd(Token)(tokens->tokens) });
   lexerAcceptFile(lexer);

   //_printTestOutput(buff, tokens, lexer);//


   data.input = stringCreate(argv[1]);
   
   data.dir = stringGetDirectory(data.input);

   data.inputFileOnly = stringGetFilename(data.input);
   sprintf(namebuff, "%s", c_str(data.inputFileOnly));
   stringSet(data.inputFileOnly, namebuff);

   sprintf(namebuff, "%s.h", c_str(data.inputFileOnly));
   data.outputh = stringCreate(namebuff);

   sprintf(namebuff, "%s.c", c_str(data.inputFileOnly));
   data.outputc = stringCreate(namebuff);


   //prefixing the output with  directories sucks, should add a path combine funciton
   if(stringLen(data.dir) > 0){
      String *dircpy = stringCopy(data.dir);
      stringConcatChar(dircpy, '/');
      stringConcat(dircpy, c_str(data.outputh));
      stringSet(data.outputh, c_str(dircpy));

      stringSet(dircpy, c_str(data.dir));
      stringConcatChar(dircpy, '/');
      stringConcat(dircpy, c_str(data.outputc));
      stringSet(data.outputc, c_str(dircpy));

      stringDestroy(dircpy);
   }

   printf("Output: %s; %s\n", c_str(data.outputh), c_str(data.outputc));

   data.structs = lexer->structs;

   createHeader(&data);
   createSource(&data);

   stringDestroy(data.input);
   stringDestroy(data.inputFileOnly);
   stringDestroy(data.dir);
   stringDestroy(data.outputh);
   stringDestroy(data.outputc);
   checkedFree(buff);
   tokenizerDestroy(tokens);
   lexerDestroy(lexer);

   printMemoryLeaks();
   //hmm
   buff;
   return 0;
}