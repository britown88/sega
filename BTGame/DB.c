#include "DB.h"
#include "WorldView.h"
#include "segautils/String.h"
#include "segashared/CheckedMemory.h"
#include "Console.h"
#include "segautils/BitBuffer.h"

#include "sqlite/sqlite3.h"

struct DB_t {
   WorldView *view;
   String *dbPath, *err;
   sqlite3 *conn;
   bool open;
};

DB *DBCreate(WorldView *view){
   
   DB *out = checkedCalloc(1, sizeof(DB));
   out->conn = NULL;
   out->dbPath = stringCreate("");
   out->view = view;
   out->open = false;
   out->err = stringCreate("");

   return out;
}

int DBConnect(DB *self, const char *filename) {
   int result = 0;
   
   if (!self || self->open) {

      if (stringEqualRaw(self->dbPath, filename)) {
         //attempting to reconnect to existing db, no-op
         return 0;
      }
      else {
         //already open bro
         stringSet(self->err, "Database already open.");
         return 1;
      }
   }
   
   result = sqlite3_open_v2(filename, &self->conn, SQLITE_OPEN_READWRITE, NULL);

   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      sqlite3_close(self->conn);
      self->conn = NULL;
      return 1;
   }

   self->open = true;
   stringSet(self->dbPath, filename);
   return 0;

}

int DBDisconnect(DB *self) {
   if (!self || !self->open) {
      stringSet(self->err, "No db connection.");
      return 1;
   }

   //TODO: destroy prepared procs
   sqlite3_close(self->conn);

   self->open = false;
   self->conn = NULL;
   return 0;
}

void DBDestroy(DB *self) {
   if (self) {      
      if (self->open) {
         DBDisconnect(self);
      }

      stringDestroy(self->dbPath);
      stringDestroy(self->err);
      
      checkedFree(self);
   }
}

const char *DBGetError(DB *self) {
   return c_str(self->err);
}

int DBInsertImage(DB *self, StringView id, const char *path) {
   const char *stmt = "INSERT INTO Images VALUES (:id, :image);";
   sqlite3_stmt *sqlstmt = NULL;
   int result = 0;
   
   if (!self || !self->open) {
      stringSet(self->err, "No db connection.");
      return 1;
   }

   result = sqlite3_prepare_v2(self->conn, stmt, strlen(stmt) + 1, &sqlstmt, NULL);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }
   
   result = sqlite3_bind_text(sqlstmt, 1, id, -1, NULL);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }
   
   {
      long size = 0;
      byte *buff = readFullFile(path, &size);

      if (!buff) {         
         stringSet(self->err, "Failure inserting image.  File not found: ");
         stringConcat(self->err, path);
         return 1;
      }

      result = sqlite3_bind_blob(sqlstmt, 2, buff, size, NULL);
      if (result != SQLITE_OK) {
         stringSet(self->err, sqlite3_errmsg(self->conn));
         return 1;
      }
   }

   result = sqlite3_step(sqlstmt);
   if (result != SQLITE_DONE) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }

   result = sqlite3_finalize(sqlstmt);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }

   return 0;

}

int DBSelectImage(DB *self, StringView id, byte **buffer, int *size) {
   const char *stmt = "SELECT image FROM Images WHERE id=:id;";
   sqlite3_stmt *sqlstmt = NULL;
   int result = 0;
   void *blob;

   if (!self || !self->open) {
      stringSet(self->err, "No db connection.");
      return 1;
   }

   result = sqlite3_prepare_v2(self->conn, stmt, strlen(stmt) + 1, &sqlstmt, NULL);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }

   result = sqlite3_bind_text(sqlstmt, 1, id, -1, NULL);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }

   result = sqlite3_step(sqlstmt);
   if (result != SQLITE_ROW) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }

   *size = sqlite3_column_bytes(sqlstmt, 0);

   if (*size == 0) {
      stringSet(self->err, "No data found.");
      return 1;
   }

   blob = sqlite3_column_blob(sqlstmt, 0);

   if (!blob) {
      stringSet(self->err, "No data found.");
      return 1;
   }

   *buffer = checkedCalloc(1, *size);
   memcpy(*buffer, blob, *size);

   result = sqlite3_finalize(sqlstmt);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }

   return 0;
}
