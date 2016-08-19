/***********************************************************************
   WARNING: This file generated by robots.  Do not attempt to modify.

   This API is for use with test.db
   Which contains 2 table(s).
***********************************************************************/

#include "test.h"
#include "DB.h"
#include "segashared/CheckedMemory.h"
#include "sqlite/sqlite3.h"

void dbImageDestroyStatements(DB_test *db);
int dbImageCreateTable(DB_test *db);
void dbPaletteDestroyStatements(DB_test *db);
int dbPaletteCreateTable(DB_test *db);

typedef struct {
   sqlite3_stmt *insert;
   sqlite3_stmt *selectAll;
   sqlite3_stmt *deleteAll;
   sqlite3_stmt *selectByid;
   sqlite3_stmt *deleteByid;
   sqlite3_stmt *selectByfoo;
   sqlite3_stmt *deleteByfoo;
} DBImageStmts;

typedef struct {
   sqlite3_stmt *insert;
   sqlite3_stmt *selectAll;
   sqlite3_stmt *deleteAll;
   sqlite3_stmt *selectByid;
   sqlite3_stmt *deleteByid;
} DBPaletteStmts;

struct DB_test{
   DBBase base;
   DBImageStmts ImageStmts;
   DBPaletteStmts PaletteStmts;
};

DB_test *db_testCreate(){
   DB_test *out = checkedCalloc(1, sizeof(DB_test));
   return out;
}

void db_testDestroy(DB_test *self){
   dbDestroy((DBBase*)self);
   dbImageDestroyStatements(self);
   dbPaletteDestroyStatements(self);
   checkedFree(self);
}

int db_testCreateTables(DB_test *self){
   int result = 0;

   if((result = dbImageCreateTable(self)) != DB_SUCCESS){ return result; }
   if((result = dbPaletteCreateTable(self)) != DB_SUCCESS){ return result; }
}

#define VectorTPart DBImage
#include "segautils/Vector_Impl.h"

void dbImageDestroy(DBImage *self){
   if(self->id){
      stringDestroy(self->id);
      self->id = NULL;
   }
   if(self->image){
      checkedFree(self->image);
      self->image = NULL;
   }
}
void dbImageDestroyStatements(DB_test *db){
   if(db->ImageStmts.insert){
      sqlite3_finalize(db->ImageStmts.insert);
      db->ImageStmts.insert = NULL;
   }
   if(db->ImageStmts.selectAll){
      sqlite3_finalize(db->ImageStmts.selectAll);
      db->ImageStmts.selectAll = NULL;
   }
   if(db->ImageStmts.deleteAll){
      sqlite3_finalize(db->ImageStmts.deleteAll);
      db->ImageStmts.deleteAll = NULL;
   }
   if(db->ImageStmts.selectByid){
      sqlite3_finalize(db->ImageStmts.selectByid);
      db->ImageStmts.selectByid = NULL;
   }
   if(db->ImageStmts.deleteByid){
      sqlite3_finalize(db->ImageStmts.deleteByid);
      db->ImageStmts.deleteByid = NULL;
   }
   if(db->ImageStmts.selectByfoo){
      sqlite3_finalize(db->ImageStmts.selectByfoo);
      db->ImageStmts.selectByfoo = NULL;
   }
   if(db->ImageStmts.deleteByfoo){
      sqlite3_finalize(db->ImageStmts.deleteByfoo);
      db->ImageStmts.deleteByfoo = NULL;
   }
}
int dbImageCreateTable(DB_test *db){
   static const char *cmd = "CREATE TABLE \"Image\" (\"id\" STRING PRIMARY KEY UNIQUE ON CONFLICT REPLACE, \"image\" BLOB, \"foo\" INT);";
   return dbExecute((DBBase*)db, cmd);
}
int dbImageInsert(DB_test *db, const DBImage *obj){
   int result = 0;
   static const char *stmt = "INSERT INTO \"Image\" (\"id\", \"image\", \"foo\") VALUES (:id, :image, :foo);";
   if(dbPrepareStatement((DBBase*)db, &db->ImageStmts.insert, stmt) != DB_SUCCESS){
      return DB_FAILURE;
   }

   //bind the values
   result = sqlite3_bind_text(db->ImageStmts.insert, 1, c_str(obj->id), -1, NULL);
   if (result != SQLITE_OK) {
      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));
      return DB_FAILURE;
   }

   result = sqlite3_bind_blob(db->ImageStmts.insert, 2, obj->image, obj->imageSize, SQLITE_TRANSIENT);
   if (result != SQLITE_OK) {
      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));
      return DB_FAILURE;
   }

   result = sqlite3_bind_int(db->ImageStmts.insert, 3, (int)obj->foo);
   if (result != SQLITE_OK) {
      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));
      return DB_FAILURE;
   }

   //now run it
   result = sqlite3_step(db->ImageStmts.insert);
   if (result != SQLITE_DONE) {
      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));
      return DB_FAILURE;
   }

   return DB_SUCCESS;
}
vec(DBImage) *dbImageSelectAll(DB_test *db){
   int result = 0;
   static const char *stmt = "SELECT * FROM \"Image\";";
   if(dbPrepareStatement((DBBase*)db, &db->ImageStmts.selectAll, stmt) != DB_SUCCESS){
      return NULL;
   }

   vec(DBImage) *out = vecCreate(DBImage)(&dbImageDestroy);

   while((result = sqlite3_step(db->ImageStmts.selectAll)) == SQLITE_ROW){
      DBImage newObj = {0};

      newObj.id = stringCreate(sqlite3_column_text(db->ImageStmts.selectAll, 0));
      newObj.imageSize = sqlite3_column_bytes(db->ImageStmts.selectAll, 1);
      newObj.image = checkedCalloc(1, newObj.imageSize);
      memcpy(newObj.image, sqlite3_column_blob(db->ImageStmts.selectAll, 1), newObj.imageSize);
      newObj.foo = sqlite3_column_int(db->ImageStmts.selectAll, 2);
      
      vecPushBack(DBImage)(out, &newObj);

   };

   if(result != SQLITE_DONE){
      vecDestroy(DBImage)(out);
      return NULL;
   }

   return out;
}
DBImage dbImageSelectFirstByid(DB_test *db, const char *id){

}
DBImage dbImageSelectFirstByfoo(DB_test *db, int foo){

}
vec(DBImage) *dbImageSelectByfoo(DB_test *db, int foo){

}
void dbImageDeleteAll(DB_test *db){

}
void dbImageDeleteByid(DB_test *db, const char *id){

}
void dbImageDeleteByfoo(DB_test *db, int foo){

}
#define VectorTPart DBPalette
#include "segautils/Vector_Impl.h"

void dbPaletteDestroy(DBPalette *self){
   if(self->id){
      stringDestroy(self->id);
      self->id = NULL;
   }
   if(self->palette){
      checkedFree(self->palette);
      self->palette = NULL;
   }
}
void dbPaletteDestroyStatements(DB_test *db){
   if(db->PaletteStmts.insert){
      sqlite3_finalize(db->PaletteStmts.insert);
      db->PaletteStmts.insert = NULL;
   }
   if(db->PaletteStmts.selectAll){
      sqlite3_finalize(db->PaletteStmts.selectAll);
      db->PaletteStmts.selectAll = NULL;
   }
   if(db->PaletteStmts.deleteAll){
      sqlite3_finalize(db->PaletteStmts.deleteAll);
      db->PaletteStmts.deleteAll = NULL;
   }
   if(db->PaletteStmts.selectByid){
      sqlite3_finalize(db->PaletteStmts.selectByid);
      db->PaletteStmts.selectByid = NULL;
   }
   if(db->PaletteStmts.deleteByid){
      sqlite3_finalize(db->PaletteStmts.deleteByid);
      db->PaletteStmts.deleteByid = NULL;
   }
}
int dbPaletteCreateTable(DB_test *db){
   static const char *cmd = "CREATE TABLE \"Palette\" (\"id\" STRING PRIMARY KEY UNIQUE ON CONFLICT REPLACE, \"palette\" BLOB);";
   return dbExecute((DBBase*)db, cmd);
}
int dbPaletteInsert(DB_test *db, const DBPalette *obj){
   int result = 0;
   static const char *stmt = "INSERT INTO \"Palette\" (\"id\", \"palette\") VALUES (:id, :palette);";
   if(dbPrepareStatement((DBBase*)db, &db->PaletteStmts.insert, stmt) != DB_SUCCESS){
      return DB_FAILURE;
   }

   //bind the values
   result = sqlite3_bind_text(db->PaletteStmts.insert, 1, c_str(obj->id), -1, NULL);
   if (result != SQLITE_OK) {
      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));
      return DB_FAILURE;
   }

   result = sqlite3_bind_blob(db->PaletteStmts.insert, 2, obj->palette, obj->paletteSize, SQLITE_TRANSIENT);
   if (result != SQLITE_OK) {
      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));
      return DB_FAILURE;
   }

   //now run it
   result = sqlite3_step(db->PaletteStmts.insert);
   if (result != SQLITE_DONE) {
      stringSet(db->base.err, sqlite3_errmsg(db->base.conn));
      return DB_FAILURE;
   }

   return DB_SUCCESS;
}
vec(DBPalette) *dbPaletteSelectAll(DB_test *db){
   int result = 0;
   static const char *stmt = "SELECT * FROM \"Palette\";";
   if(dbPrepareStatement((DBBase*)db, &db->PaletteStmts.selectAll, stmt) != DB_SUCCESS){
      return NULL;
   }

   vec(DBPalette) *out = vecCreate(DBPalette)(&dbPaletteDestroy);

   while((result = sqlite3_step(db->PaletteStmts.selectAll)) == SQLITE_ROW){
      DBPalette newObj = {0};

      newObj.id = stringCreate(sqlite3_column_text(db->PaletteStmts.selectAll, 0));
      newObj.paletteSize = sqlite3_column_bytes(db->PaletteStmts.selectAll, 1);
      newObj.palette = checkedCalloc(1, newObj.paletteSize);
      memcpy(newObj.palette, sqlite3_column_blob(db->PaletteStmts.selectAll, 1), newObj.paletteSize);
      
      vecPushBack(DBPalette)(out, &newObj);

   };

   if(result != SQLITE_DONE){
      vecDestroy(DBPalette)(out);
      return NULL;
   }

   return out;
}
DBPalette dbPaletteSelectFirstByid(DB_test *db, const char *id){

}
void dbPaletteDeleteAll(DB_test *db){

}
void dbPaletteDeleteByid(DB_test *db, const char *id){

}
