#include "sqliteWrapper.h"

namespace sqliteWRAP {
#include <winsqlite/winsqlite3.h>
}

int sqlite3_close(sqlite3*a) { return sqliteWRAP::sqlite3_close(a); }
int sqlite3_exec(sqlite3*a, const char *b, int(*c)(void*, int, char**, char**), void *d, char **e) { return sqliteWRAP::sqlite3_exec(a, b, c, d, e); }
int sqlite3_open_v2(const char *a, sqlite3 **b, int c, const char *d) { return sqliteWRAP::sqlite3_open_v2(a, b, c, d); }
const char *sqlite3_errmsg(sqlite3*a) { return sqliteWRAP::sqlite3_errmsg(a); }
int sqlite3_prepare_v2(sqlite3 *a, const char *b, int c, sqlite3_stmt **d, const char **e) { return sqliteWRAP::sqlite3_prepare_v2(a, b, c, d, e); }
int sqlite3_reset(sqlite3_stmt *a) { return sqliteWRAP::sqlite3_reset(a); }

int sqlite3_finalize(sqlite3_stmt *a) { return sqliteWRAP::sqlite3_finalize(a); }
int sqlite3_bind_text(sqlite3_stmt*a, int b, const char*c, int d, void(*e)(void*)) { return sqliteWRAP::sqlite3_bind_text(a, b, c, d, e); }
int sqlite3_bind_blob(sqlite3_stmt*a, int b, const void*c, int d, void(*e)(void*)) { return sqliteWRAP::sqlite3_bind_blob(a, b, c, d, e); }
int sqlite3_step(sqlite3_stmt*a) { return sqliteWRAP::sqlite3_step(a); }
const unsigned char *sqlite3_column_text(sqlite3_stmt*a, int b) { return sqliteWRAP::sqlite3_column_text(a, b); }
int sqlite3_column_bytes(sqlite3_stmt*a, int b) { return sqliteWRAP::sqlite3_column_bytes(a, b); }
const void *sqlite3_column_blob(sqlite3_stmt*a, int b) { return sqliteWRAP::sqlite3_column_blob(a, b); }
int sqlite3_bind_int(sqlite3_stmt*a, int b, int c) { return sqliteWRAP::sqlite3_bind_int(a, b, c); }
int sqlite3_column_int(sqlite3_stmt*a, int b) { return sqliteWRAP::sqlite3_column_int(a, b); }