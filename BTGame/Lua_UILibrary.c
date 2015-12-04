#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Console.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"


static int slua_textAreaPush(lua_State *L);
static int slua_textAreaHide(lua_State *L);
static int slua_textAreaShow(lua_State *L);
static int slua_textAreaSetVisibility(lua_State *L);

static int slua_textAreaAddTextArea(lua_State *L) {
   const char *name = lua_tostring(L, 1);
   StringView *sv = lua_touserdata(L, 2);   
   
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
   luaPushFunctionTable(L, "hide", &slua_textAreaHide);
   luaPushFunctionTable(L, "show", &slua_textAreaShow);
   luaPushFunctionTable(L, "setVisibility", &slua_textAreaSetVisibility);
   lua_setglobal(L, LLIB_TEXT_AREA);
}

static StringView _nameFromTable(WorldView *view, lua_State *L) {
   StringView name = NULL;
   
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
      lua_pushliteral(L, "Failed to push text. Invalid name argument.");
      lua_error(L);
   }

   return name;
}

int slua_textAreaPush(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *msg = luaL_checkstring(L, 2);
   StringView name = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   name = _nameFromTable(view, L);

   if (textBoxManagerPushText(view->managers->textBoxManager, name, msg)) {
      lua_pushliteral(L, "Failed to push text.  Name not found.");
      lua_error(L);
   }

   return 0;
}


int slua_textAreaHide(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   StringView name = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);
   name = _nameFromTable(view, L);

   if (textBoxManagerHideTextArea(view->managers->textBoxManager, name)) {
      lua_pushliteral(L, "Failed to hide text area.  Name not found.");
      lua_error(L);
   }

   return 0;
}

int slua_textAreaShow(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   StringView name = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);
   name = _nameFromTable(view, L);

   if (textBoxManagerShowTextArea(view->managers->textBoxManager, name)) {
      lua_pushliteral(L, "Failed to show text area.  Name not found.");
      lua_error(L);
   }

   return 0;
}

int slua_textAreaSetVisibility(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   StringView name = NULL;
   bool show = lua_toboolean(L, 2);
   luaL_checktype(L, 1, LUA_TTABLE);
   name = _nameFromTable(view, L);

   if (textBoxManagerSetTextAreaVisibility(view->managers->textBoxManager, name, show)) {
      lua_pushliteral(L, "Failed to hide text area.  Name not found.");
      lua_error(L);
   }

   return 0;
}