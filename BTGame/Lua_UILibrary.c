#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_textAreaPush(lua_State *L);

void luaUIAddTextArea(lua_State *L, StringView name) {

   lua_getglobal(L, "TextAreas");
   lua_pushstring(L, name); 

   if (luaL_dostring(L, "return New(TextArea)")) { lua_error(L);}
   luaPushUserDataTable(L, "name", (void*)name);

   lua_rawset(L, -3);
   lua_pop(L, 1);
}

void luaLoadUILibrary(lua_State *L) {
   if (luaL_dofile(L, "assets/lua/lib/ui.lua")) {
      lua_pop(L, 1);
      return;
   }

   lua_getglobal(L, "TextArea");
   luaPushFunctionTable(L, "push", &slua_textAreaPush);
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