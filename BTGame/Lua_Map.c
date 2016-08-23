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
static int slua_mapSetSchemas(lua_State *L);
static int slua_mapAmbient(lua_State *L);



void luaLoadMapLibrary(lua_State *L) {
   lua_newtable(L);
   luaPushFunctionTable(L, "new", &slua_mapNew);
   luaPushFunctionTable(L, "resize", &slua_mapResize);
   luaPushFunctionTable(L, "load", &slua_mapLoad);
   luaPushFunctionTable(L, "save", &slua_mapSave);
   luaPushFunctionTable(L, "setSchemas", &slua_mapSetSchemas);
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
int slua_mapSetSchemas(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int len, i;
   luaL_checktype(L, 1, LUA_TTABLE);
   
   lua_len(L, 1);
   len = (int)lua_tointeger(L, -1);
   lua_pop(L, 1);

   gridManagerClearSchemas(view->gridManager);

   for (i = 0; i < len; ++i) {
      TileSchema *schema = gridManagerGetSchema(view->gridManager, i);

      int j;

      lua_pushinteger(L, i + 1);
      lua_gettable(L, -2);//push the schema table

      lua_pushliteral(L, "occlusion");
      lua_gettable(L, -2);
      if (!lua_isnil(L, -1)) {
         schema->occlusion = (byte)lua_tointeger(L, -1);
      }
      lua_pop(L, 1);

      lua_pushliteral(L, "lit");
      lua_gettable(L, -2);
      if (!lua_isnil(L, -1)) {
         schema->lit = (byte)lua_toboolean(L, -1);
      }
      lua_pop(L, 1);

      lua_pushliteral(L, "radius");
      lua_gettable(L, -2);
      if (!lua_isnil(L, -1)) {
         schema->radius = (byte)lua_tointeger(L, -1);
         schema->lit = true;
      }
      lua_pop(L, 1);

      lua_pushliteral(L, "centerLevel");
      lua_gettable(L, -2);
      if (!lua_isnil(L, -1)) {
         schema->centerLevel = (byte)lua_tointeger(L, -1);
         schema->lit = true;
      }
      lua_pop(L, 1);

      lua_pushliteral(L, "fadeWidth");
      lua_gettable(L, -2);
      if (!lua_isnil(L, -1)) {
         schema->fadeWidth = (byte)lua_tointeger(L, -1);
         schema->lit = true;
      }
      lua_pop(L, 1);

      lua_pushliteral(L, "img");
      lua_gettable(L, -2);
      luaL_checktype(L, -1, LUA_TTABLE);

      //get the len
      lua_len(L, -1);
      schema->imgCount = (byte)MIN(3, lua_tointeger(L, -1));
      lua_pop(L, 1);

      for (j = 0; j < schema->imgCount; ++j) {
         lua_pushinteger(L, j + 1);
         lua_gettable(L, -2);
         schema->img[j] = (short)lua_tointeger(L, -1);
         lua_pop(L, 1);
      }

      lua_pop(L, 2);//pop the img table and the schema
   }

   return 0;
}

int slua_mapAmbient(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   byte level = (byte)MAX(0, MIN(MAX_BRIGHTNESS, (int)luaL_checkinteger(L, 1)));

   gridManagerSetAmbientLight(view->gridManager, level);

   return 0;
}
