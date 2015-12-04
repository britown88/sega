#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Console.h"
#include "Map.h"
#include "GridManager.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_mapLoad(lua_State *L);
static int slua_mapSave(lua_State *L);

void luaLoadMapLibrary(lua_State *L) {
   lua_newtable(L);
   luaPushFunctionTable(L, "load", &slua_mapLoad);
   luaPushFunctionTable(L, "save", &slua_mapSave);
   lua_setglobal(L, LLIB_MAP);
}

int slua_mapLoad(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *path = luaL_checkstring(L, 1);
   Map *map = mapLoad(path);

   if (!map) {
      lua_pushliteral(L, "Failed to read map file.");
      lua_error(L);
   }

   gridManagerLoadMap(view->managers->gridManager, map);
   return 0;
}

int slua_mapSave(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *path = luaL_checkstring(L, 1);
   Map *map = gridManagerGetMap(view->managers->gridManager);

   if (!map) {
      lua_pushliteral(L, "No map currently loaded?");
      lua_error(L);
   }

   if (mapSave(map, path)) {
      lua_pushliteral(L, "Failed to save map file.");
      lua_error(L);
   }
   
   return 0;
}
