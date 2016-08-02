#include "Lua.h"
#include "Console.h"
#include "DB.h"

#include "WorldView.h"
#include "Managers.h"
#include "GameState.h"
#include "ImageLibrary.h"
#include "SEGA/App.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"


static int slua_DBConnect(lua_State *L);
static int slua_DBDisconnect(lua_State *L);
static int slua_DBInsertImage(lua_State *L);


void luaLoadDBLibrary(lua_State *L) {

   lua_newtable(L);
   luaPushFunctionTable(L, "connect", &slua_DBConnect);
   luaPushFunctionTable(L, "disconnect", &slua_DBDisconnect);
   luaPushFunctionTable(L, "insertImage", &slua_DBInsertImage);
   lua_setglobal(L, LLIB_DB);

}

int slua_DBConnect(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *dbPath = lua_tostring(L, 1);

   if (DBConnect(view->db, dbPath)) {
      lua_pushstring(L, DBGetError(view->db));
      lua_error(L);
   }

   consolePrintLine(view->console, "Connected to db: [c=0,5]%s[/c]", dbPath);
   return 0;
}

int slua_DBDisconnect(lua_State *L) {
   WorldView *view = luaGetWorldView(L);

   if (DBDisconnect(view->db)) {
      lua_pushstring(L, DBGetError(view->db));
      lua_error(L);
   }

   consolePrintLine(view->console, "Disconnected.");
   return 0;
}

int slua_DBInsertImage(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *id = lua_tostring(L, 1);
   const char *path = lua_tostring(L, 2);

   if (DBInsertImage(view->db, stringIntern(id), path)) {
      lua_pushstring(L, DBGetError(view->db));
      lua_error(L);
   }

   consolePrintLine(view->console, "Image [c=0,5]%s[/c] inserted.", id);
   return 0;
}

