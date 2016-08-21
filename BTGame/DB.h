#pragma once

#include "segashared/Strings.h"
#include "segautils/String.h"

typedef struct DB_t DB;

#define DB_SUCCESS 0
#define DB_FAILURE 1
#define DB_CREATED 2

//new shit
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

typedef struct DBBase {
   sqlite3 *conn;
   String *dbPath, *err;
   bool open;
}DBBase;

int dbConnect(DBBase *self, const char *filename, bool create);
int dbDisconnect(DBBase *self);
void dbDestroy(DBBase *self);//this does not call free on self!!
const char *dbGetError(DBBase *self);

int dbExecute(DBBase *self, const char *cmd);

//if *out is null, it will create and preapre a new statement there
//otherwise it will call reset on it
int dbPrepareStatement(DBBase *self, sqlite3_stmt **out, const char *stmt);






