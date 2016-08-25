#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Console.h"
#include "Map.h"
#include "GridManager.h"

#include "SEGA/App.h"
#include "segautils/Defs.h"
#include "LightGrid.h"
#include "DB.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_mapNew(lua_State *L);
static int slua_mapResize(lua_State *L);
static int slua_mapLoad(lua_State *L);
static int slua_mapSave(lua_State *L);
static int slua_mapsetTileSchema(lua_State *L);
static int slua_mapAmbient(lua_State *L);



void luaLoadMapLibrary(lua_State *L) {
   lua_newtable(L);
   luaPushFunctionTable(L, "new", &slua_mapNew);
   luaPushFunctionTable(L, "resize", &slua_mapResize);
   luaPushFunctionTable(L, "load", &slua_mapLoad);
   luaPushFunctionTable(L, "save", &slua_mapSave);
   luaPushFunctionTable(L, "setTileSchema", &slua_mapsetTileSchema);
   luaPushFunctionTable(L, "setAmbient", &slua_mapAmbient);
   lua_setglobal(L, LLIB_MAP);
}

int slua_mapNew(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int x = (int)luaL_checkinteger(L, 1);
   int y = (int)luaL_checkinteger(L, 2);
   Map *map = mapCreate(x, y);
   gridManagerLoadMap(view->gridManager, map);
   return 0;
}

int slua_mapResize(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int x = (int)luaL_checkinteger(L, 1);
   int y = (int)luaL_checkinteger(L, 2);
   Map *map = gridManagerGetMap(view->gridManager);
   mapResize(map, x, y);
   gridManagerLoadMap(view->gridManager, map);
   return 0;
}

int slua_mapLoad(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *path = luaL_checkstring(L, 1);
   Map *map = mapLoad(path);

   if (!map) {
      lua_pushliteral(L, "Failed to read map file.");
      lua_error(L);
   }

   gridManagerLoadMap(view->gridManager, map);
   return 0;
}
int slua_mapSave(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *path = luaL_checkstring(L, 1);
   Map *map = gridManagerGetMap(view->gridManager);

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
int slua_mapsetTileSchema(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *set = luaL_checkstring(L, 1);
   gridManagerLoadSchemaTable(view->gridManager, set);
   return 0;
}

int slua_mapAmbient(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   byte level = (byte)MAX(0, MIN(MAX_BRIGHTNESS, (int)luaL_checkinteger(L, 1)));

   gridManagerSetAmbientLight(view->gridManager, level);

   return 0;
}
