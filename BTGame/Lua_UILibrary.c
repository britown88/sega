#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_textAreaPush(lua_State *L);

static void addTextArea(lua_State *L, const char *name) {
   lua_pushstring(L, name);
   lua_pushliteral(L, "get");
   lua_gettable(L, 1);
   lua_pushvalue(L, 1);
   lua_call(L, 1, 1);
   lua_pushliteral(L, "name");
   lua_pushlightuserdata(L, stringIntern(name));
   lua_rawset(L, -3);
   lua_rawset(L, -3);
}

void luaLoadUILibrary(lua_State *L) {
   int result;

   lua_newtable(L);

   lua_pushliteral(L, "name");
   lua_pushlightuserdata(L, NULL);
   lua_rawset(L, -3);

   luaPushFunctionTable(L, "get", &luaNewObject);
   luaPushFunctionTable(L, "push", &slua_textAreaPush);

   lua_newtable(L);
   
   addTextArea(L, "smallbox");

   lua_setglobal(L, "TextAreas");

   lua_pop(L, 1);

}

int slua_textAreaPush(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *msg = luaL_checkstring(L, 2);
   StringView name = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   //get name
   lua_pushliteral(L, "name");
   lua_gettable(L, 1);
   name = lua_touserdata(L, -1);
   lua_pop(L, 1);

   if (!name) {
      lua_pushliteral(L, "Name not found.");
      lua_error(L);
   }

   textBoxManagerPushText(view->managers->textBoxManager, name, msg);
   return 0;
}