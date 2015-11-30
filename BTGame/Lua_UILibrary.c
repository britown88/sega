#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Console.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"


static int slua_textAreaPush(lua_State *L);

static int slua_textAreaAddTextArea(lua_State *L) {
   StringView *sv = lua_touserdata(L, 1);
   const char *name = lua_tostring(L, 2);
   
   lua_getglobal(L, LLIB_TEXT_AREAS);
   lua_pushstring(L, name);

   lua_pushcfunction(L, &luaNewObject);
   lua_getglobal(L, LLIB_TEXT_AREA);
   lua_call(L, 1, 1);

   luaPushUserDataTable(L, "name", (void*)sv);

   lua_settable(L, -3);
   lua_pop(L, 1);
   return 0;
}

void luaUIAddTextArea(lua_State *L, StringView name) {
   lua_pushcfunction(L, &slua_textAreaAddTextArea);
   lua_pushstring(L, name);
   lua_pushlightuserdata(L, (void*)name);
   if (lua_pcall(L, 2, 0, 0)) {
      WorldView *view = luaGetWorldView(L);
      consolePrintLuaError(view->console, "Error adding text area");
   }
   
}

void luaLoadUILibrary(lua_State *L) {
   lua_newtable(L);
   lua_setglobal(L, LLIB_TEXT_AREAS);
   
   lua_newtable(L);
   luaPushFunctionTable(L, "push", &slua_textAreaPush);
   lua_setglobal(L, LLIB_TEXT_AREA);
}

int slua_textAreaPush(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *msg = luaL_checkstring(L, 2);
   StringView name = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   //get name
   lua_pushliteral(L, "name");
   lua_gettable(L, 1);
   if (lua_isuserdata(L, -1)) {
      name = lua_touserdata(L, -1);
   }
   else if (lua_isstring(L, -1)) {
      name = stringIntern(luaL_checkstring(L, -1));
   }   
   lua_pop(L, 1);

   if (!name) {
      lua_pushliteral(L, "Name not found.");
      lua_error(L);
   }

   textBoxManagerPushText(view->managers->textBoxManager, name, msg);
   return 0;
}