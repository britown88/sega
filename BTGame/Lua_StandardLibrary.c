#include "Lua.h"
#include "Console.h"

#include "WorldView.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_consolePrint(lua_State *L);
static int slua_consoleClear(lua_State *L);

void luaLoadStandardLibrary(lua_State *L) {
   lua_newtable(L);
   luaPushFunctionTable(L, "print", &slua_consolePrint);
   luaPushFunctionTable(L, "clear", &slua_consoleClear);
   lua_setglobal(L, "Console");

   if (luaL_dofile(L, "assets/lua/lib/standard.lua")) {
      lua_pop(L, 1);
   }
}

int slua_consolePrint(lua_State *L) {
   const char *str = luaL_checkstring(L, 1);
   WorldView *view = luaGetWorldView(L);
   consolePushLine(view->console, str);
   return 0;
}

int slua_consoleClear(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   consoleClear(view->console);
   return 0;
}