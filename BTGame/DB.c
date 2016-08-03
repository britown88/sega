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
   sqlite3_stmt *insertPalette, *selectPalette;
};

DB *DBCreate(WorldView *view){
   
   DB *out = checkedCalloc(1, sizeof(DB));
   out->dbPath = stringCreate("");
   out->view = view;
   out->err = stringCreate("");

   return out;
}

static int _executeFile(DB *self, const char *file) {
   char *err = NULL;
   long fSize = 0;
   byte *fullFile = readFullFile(file, &fSize);
   int result = 0;

   if (!fullFile) {
      stringSet(self->err, "Unable to open sql file for execution.");
      return DB_FAILURE;
   }

   result = sqlite3_exec(self->conn, fullFile, NULL, NULL, &err);
   checkedFree(fullFile);

   if (result != SQLITE_OK) {
      stringSet(self->err, err);
      free(err);
      return DB_FAILURE;
   }

   return DB_SUCCESS;

}

int DBConnect(DB *self, const char *filename) {
   int result = 0;
   
   if (!self || self->open) {

      if (stringEqualRaw(self->dbPath, filename)) {
         //attempting to reconnect to existing db, no-op
         return DB_SUCCESS;
      }
      else {
         //already open bro
         stringSet(self->err, "Database already open.");
         return DB_FAILURE;
      }
   }
   
   result = sqlite3_open_v2(filename, &self->conn, SQLITE_OPEN_READWRITE, NULL);

   if (result != SQLITE_OK) {

      #ifdef _DEBUG

      sqlite3_close(self->conn);
      result = sqlite3_open_v2(filename, &self->conn, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
      if (result == SQLITE_OK) {
         //in debug if we try to connect to a db that doesnt exist let's make a new one!
         result = _executeFile(self, "db/db_create.sql");
         if (result == SQLITE_OK) {
            //success!
            self->open = true;
            stringSet(self->dbPath, filename);

            return DB_CREATED;
         }
         else {
            sqlite3_close(self->conn);
            self->conn = NULL;
            return DB_FAILURE;
         }
      }

      #endif

      stringSet(self->err, sqlite3_errmsg(self->conn));
      sqlite3_close(self->conn);
      self->conn = NULL;
      return DB_FAILURE;
   }

   self->open = true;
   stringSet(self->dbPath, filename);
   return DB_SUCCESS;

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
      return DB_FAILURE;
   }

   _destroyStmt(self, &self->insertImage);
   _destroyStmt(self, &self->selectImage);
   _destroyStmt(self, &self->insertPalette);
   _destroyStmt(self, &self->selectPalette);

   sqlite3_close(self->conn);

   self->open = false;
   self->conn = NULL;
   return DB_SUCCESS;
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
      return DB_FAILURE;
   }

   if (!*out) {
      sqlite3_stmt *sqlstmt = NULL;
      int result = sqlite3_prepare_v2(self->conn, stmt, strlen(stmt) + 1, &sqlstmt, NULL);
      if (result != SQLITE_OK) {
         stringSet(self->err, sqlite3_errmsg(self->conn));
         return DB_FAILURE;
      }
      *out = sqlstmt;
   }

   sqlite3_reset(*out);
   return DB_SUCCESS;
}

static void _destroyBlob(void*data) {

}


int DBInsertImage(DB *self, StringView id, const char *path) {
   int result = 0;

   if (_prepare(self, &self->insertImage, "INSERT INTO Images VALUES (:id, :image);")) {
      return DB_FAILURE;
   }
   
   
   result = sqlite3_bind_text(self->insertImage, 1, id, -1, NULL);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return DB_FAILURE;
   }
   
   {
      long size = 0;
      byte *buff = readFullFile(path, &size);

      if (!buff) {         
         stringSet(self->err, "Failure inserting image.  File not found: ");
         stringConcat(self->err, path);
         return DB_FAILURE;
      }

      result = sqlite3_bind_blob(self->insertImage, 2, buff, size, SQLITE_TRANSIENT);
      checkedFree(buff);
      if (result != SQLITE_OK) {
         stringSet(self->err, sqlite3_errmsg(self->conn));
         return DB_FAILURE;
      }
   }

   result = sqlite3_step(self->insertImage);
   if (result != SQLITE_DONE) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return DB_FAILURE;
   }

   return DB_SUCCESS;

}

int DBSelectImage(DB *self, StringView id, byte **buffer, int *size) {
   int result = 0;
   void *blob = NULL;

   if (_prepare(self, &self->selectImage, "SELECT image FROM Images WHERE id=:id;")) {
      return DB_FAILURE;
   }

   result = sqlite3_bind_text(self->selectImage, 1, id, -1, NULL);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return DB_FAILURE;
   }

   result = sqlite3_step(self->selectImage);
   if (result != SQLITE_ROW) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return DB_FAILURE;
   }

   *size = sqlite3_column_bytes(self->selectImage, 0);

   if (*size == 0) {
      stringSet(self->err, "No data found.");
      return DB_FAILURE;
   }

   blob = sqlite3_column_blob(self->selectImage, 0);

   if (!blob) {
      stringSet(self->err, "No data found.");
      return DB_FAILURE;
   }

   *buffer = blob;

   return DB_SUCCESS;
}

int DBInsertPalette(DB *self, StringView id, const char *path) {
   int result = 0;

   if (_prepare(self, &self->insertPalette, "INSERT INTO Palettes VALUES (:id, :palette);")) {
      return DB_FAILURE;
   }


   result = sqlite3_bind_text(self->insertPalette, 1, id, -1, NULL);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return DB_FAILURE;
   }

   {
      Palette p = paletteDeserialize(path);

      if (!memcmp(&p, &(Palette){0}, sizeof(Palette))) {
         stringSet(self->err, "Failure inserting Palette.  File not found. ");
         stringConcat(self->err, path);
         return DB_FAILURE;
      }

      result = sqlite3_bind_blob(self->insertPalette, 2, &p, sizeof(Palette), SQLITE_TRANSIENT);

      if (result != SQLITE_OK) {
         stringSet(self->err, sqlite3_errmsg(self->conn));
         return DB_FAILURE;
      }
   }

   result = sqlite3_step(self->insertPalette);
   if (result != SQLITE_DONE) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return DB_FAILURE;
   }

   return DB_SUCCESS;

}

int DBSelectPalette(DB *self, StringView id, byte **buffer, int *size) {
   int result = 0;
   void *blob = NULL;

   if (_prepare(self, &self->selectPalette, "SELECT palette FROM Palettes WHERE id=:id;")) {
      return DB_FAILURE;
   }

   result = sqlite3_bind_text(self->selectPalette, 1, id, -1, NULL);
   if (result != SQLITE_OK) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return DB_FAILURE;
   }

   result = sqlite3_step(self->selectPalette);
   if (result != SQLITE_ROW) {
      stringSet(self->err, sqlite3_errmsg(self->conn));
      return DB_FAILURE;
   }

   *size = sqlite3_column_bytes(self->selectPalette, 0);

   if (*size == 0) {
      stringSet(self->err, "No data found.");
      return DB_FAILURE;
   }

   blob = sqlite3_column_blob(self->selectPalette, 0);

   if (!blob) {
      stringSet(self->err, "No data found.");
      return DB_FAILURE;
   }

   *buffer = blob;

   return DB_SUCCESS;
}


