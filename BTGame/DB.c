#include "DB.h"
#include "WorldView.h"
#include "segautils/String.h"
#include "segashared/CheckedMemory.h"
#include "Console.h"

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
      //already open bro
      stringSet(self->err, "Database already open.");
      return 1;
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
