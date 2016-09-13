#pragma once

#include "segautils/extern_c.h"

#define SQLITE_OPEN_READWRITE        0x00000002  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_CREATE           0x00000004  /* Ok for sqlite3_open_v2() */
#define SQLITE_OK           0   /* Successful result */
#define SQLITE_ROW         100  /* sqlite3_step() has another row ready */
#define SQLITE_DONE        101  /* sqlite3_step() has finished executing */

typedef void(*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)


SEXTERN_C

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

int sqlite3_close(sqlite3*);
int sqlite3_exec(sqlite3*, const char *sql, int(*callback)(void*, int, char**, char**), void *, char **);
int sqlite3_open_v2(const char *, sqlite3 **, int, const char *);
const char *sqlite3_errmsg(sqlite3*);
int sqlite3_prepare_v2(sqlite3 *, const char *, int, sqlite3_stmt **, const char **);
int sqlite3_reset(sqlite3_stmt *);
int sqlite3_finalize(sqlite3_stmt *);
int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int, void(*)(void*));
int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*));
int sqlite3_step(sqlite3_stmt*);
const unsigned char *sqlite3_column_text(sqlite3_stmt*, int );
int sqlite3_column_bytes(sqlite3_stmt*, int );
const void *sqlite3_column_blob(sqlite3_stmt*, int );
int sqlite3_bind_int(sqlite3_stmt*, int, int);
int sqlite3_column_int(sqlite3_stmt*, int );

END_SEXTERN_C
