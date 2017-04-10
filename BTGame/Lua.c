#include "Lua.h"
#include "Console.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

#include "assets.h"

lua_State *luaCreate() {
   lua_State *L = luaL_newstate();   

   return L;
}

void luaDestroy(lua_State *self) {
   lua_close(self);
}

WorldView *luaGetWorldView(lua_State *L) {
   WorldView *view;
   lua_getglobal(L, LLIB_VIEW);
   view = lua_touserdata(L, -1);
   lua_pop(L, 1);
   return view;
}

//this was a proof of concept
int luaNewObject(lua_State *L) {

   luaL_checktype(L, 1, LUA_TTABLE);
   if (lua_isnoneornil(L, 2)){
      lua_newtable(L);
   }
   //copy self to top
   lua_pushvalue(L, 1);
   lua_setmetatable(L, 2);

   //set index of self to self
   lua_pushliteral(L, "__index");
   lua_pushvalue(L, 1);
   lua_settable(L, 1);

   return 1;
}

void luaPushFunctionGlobal(lua_State *L, const char *name, lua_CFunction func) {
   lua_pushcfunction(L, func);
   lua_setglobal(L, name);
}
void luaPushFunctionTable(lua_State *L, const char *index, lua_CFunction func) {
   lua_pushstring(L, index);
   lua_pushcfunction(L, func);
   lua_settable(L, -3);
}

void luaPushUserDataGlobal(lua_State *L, const char *name, void *data) {
   lua_pushlightuserdata(L, data);
   lua_setglobal(L, name);
}
void luaPushUserDataTable(lua_State *L, const char *index, void *data) {
   lua_pushstring(L, index);
   lua_pushlightuserdata(L, data);
   lua_settable(L, -3);
}

void *luaGetUserDataFromTable(lua_State *L, int tableIndex, const char *index) {
   void *out = NULL;
   lua_pushstring(L, index);
   lua_gettable(L, tableIndex);
   if (lua_isuserdata(L, -1)) {
      out = lua_touserdata(L, -1);
   }
   lua_pop(L, 1);
   return out;
}

void luaRequire(lua_State *L, const char *modname) {
   WorldView *view = luaGetWorldView(L);
   lua_getglobal(L, "require");
   lua_pushstring(L, modname);
   if (lua_pcall(L, 1, LUA_MULTRET, 0)) {
      consolePrintLuaError(view->console, "Error loading module");
      return;
   }
   else if (!lua_toboolean(L, -1)) {
      consolePrintLine(view->console, "[c=0,13]Module %s not loaded![/c]", modname);
   }

   
   lua_pop(L, 1);
}

static int slua_loadDBScript(lua_State *L) {
   char *c = luaL_checkstring(L, 1);
   WorldView *view = luaGetWorldView(L);

   DBLuaScript script = dbLuaScriptSelectFirstBymodule(view->db, c);
   if (!script.script) {
      lua_pushstring(L, "Unable require module");
      lua_error(L);
   }

   luaL_dostring(L, c_str(script.script));
   return 0;
}

static void _addScriptLoader(lua_State *L, DBLuaScript *script) {
   
   lua_pushcfunction(L, slua_loadDBScript);
   lua_setfield(L, -2, c_str(script->module));
   
}

void luaCreateScriptLoaders(lua_State *L) {
   WorldView *view = luaGetWorldView(L);

   luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");

   vec(DBLuaScript) *scripts = dbLuaScriptSelectAll(view->db);
   vecForEach(DBLuaScript, script, scripts, {
      _addScriptLoader(L, script);      
   });

   lua_pop(L, 1);  // remove _PRELOAD table
}