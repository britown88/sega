#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_textAreaPush(lua_State *L);

void luaUIAddTextArea(lua_State *L, StringView name) {
   //push the class
   lua_getglobal(L, "TextArea"); // 1

   //push the collection
   lua_getglobal(L, "TextAreas"); // 2

   //push the name of the one we're making
   lua_pushstring(L, name); // 3

   //push the name of the get function and pull that funciton from the class
   lua_pushliteral(L, "new");
   lua_gettable(L, -4); // 4

   //copy the class to the first argument
   lua_pushvalue(L, -4);

   //call get
   lua_call(L, 1, 1);

   //set the name
   lua_pushliteral(L, "name");
   lua_pushlightuserdata(L, name);
   lua_rawset(L, -3);

   //set this new table under the wanted name inside the main table
   lua_rawset(L, -3);

   //pop the class and collection
   lua_pop(L, 2);
}

void luaLoadUILibrary(lua_State *L) {
   int result;

   lua_newtable(L);

   lua_pushliteral(L, "name");
   lua_pushlightuserdata(L, NULL);
   lua_rawset(L, -3);

   luaPushFunctionTable(L, "new", &luaNewObject);
   luaPushFunctionTable(L, "push", &slua_textAreaPush);

   lua_setglobal(L, "TextArea");

   lua_newtable(L);
   lua_setglobal(L, "TextAreas");
}

int slua_textAreaPush(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *msg = luaL_checkstring(L, 2);
   StringView name = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   //if (lua_gettop(L) != 2) {
   //   lua_pushliteral(L, "Arguments: (TextArea Self, String message)");
   //   lua_error(L);
   //}

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