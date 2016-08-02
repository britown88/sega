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

   //time for all our cool procs
   sqlite3_stmt *insertImage, *selectImage;
};

DB *DBCreate(WorldView *view){
   
   DB *out = checkedCalloc(1, sizeof(DB));
   out->dbPath = stringCreate("");
   out->view = view;
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

static void _destroyStmt(DB *self, sqlite3_stmt **stmt) {
   if (stmt) {
      sqlite3_finalize(*stmt);
      stmt = NULL;
   }
}

int DBDisconnect(DB *self) {
   if (!self || !self->open) {
      stringSet(self->err, "No db connection.");
      return 1;
   }

   _destroyStmt(self, &self->insertImage);
   _destroyStmt(self, &self->selectImage);

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

static int _prepare(DB *self, sqlite3_stmt **out, const char *stmt) {
   if (!self || !self->open) {
      stringSet(self->err, "No db connection.");
      return 1;
   }

   if (!*out) {
      sqlite3_stmt *sqlstmt = NULL;
      int result = sqlite3_prepare_v2(self->conn, stmt, strlen(stmt) + 1, &sqlstmt, NULL);
      if (result != SQLITE_OK) {
         stringSet(self->err, sqlite3_errmsg(self->conn));
         return 1;
      }
      *out = sqlstmt;
   }

   sqlite3_reset(*out);
   return 0;
}

static void _destroyBlob(void*data) {

}


int DBInsertImage(DB *self, StringView id, const char *path) {
   int result = 0;

   if (_prepare(self, &self->insertImage, "INSERT INTO Images VALUES (:id, :image);")) {
      return 1;
   }
   
   
   result = sqlite3_bind_text(self->insertImage, 1, id, -1, NULL);
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

      result = sqlite3_bind_blob(self->insertImage, 2, buff, size, SQLITE_TRANSIENT);
      checkedFree(buff);
      if (result != SQLITE_OK) {
         stringSet(self->err, sqlite3_errmsg(self->conn));
         return 1;
      }
   }

   result = sqlite3_step(self->insertImage);
   if (result != SQLITE_DONE) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }

   return 0;

}

int DBSelectImage(DB *self, StringView id, byte **buffer, int *size) {
   int result = 0;
   void *blob = NULL;

   if (_prepare(self, &self->selectImage, "SELECT image FROM Images WHERE id=:id;")) {
      return 1;
   }

   result = sqlite3_bind_text(self->selectImage, 1, id, -1, NULL);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }

   result = sqlite3_step(self->selectImage);
   if (result != SQLITE_ROW) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return 1;
   }

   *size = sqlite3_column_bytes(self->selectImage, 0);

   if (*size == 0) {
      stringSet(self->err, "No data found.");
      return 1;
   }

   blob = sqlite3_column_blob(self->selectImage, 0);

   if (!blob) {
      stringSet(self->err, "No data found.");
      return 1;
   }

   *buffer = checkedCalloc(1, *size);
   memcpy(*buffer, blob, *size);

   return 0;
}

