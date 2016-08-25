#include "Lua.h"
#include "Console.h"

#include "WorldView.h"
#include "Managers.h"
#include "GameState.h"
#include "ImageLibrary.h"
#include "Sprites.h"
#include "SEGA/App.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

#include "AssetHelpers.h"

#include "DB.h"

static int slua_consolePrint(lua_State *L);
static int slua_consoleClear(lua_State *L);
static int slua_rand(lua_State *L);
static int slua_toggleStats(lua_State *L);
static int slua_openEditor(lua_State *L);
static int slua_clearImageCache(lua_State *L);
static int slua_clearSpriteCache(lua_State *L);
static int slua_setPalette(lua_State *L);


void luaLoadStandardLibrary(lua_State *L) {

   lua_newtable(L);
   luaPushFunctionTable(L, "print", &slua_consolePrint);
   luaPushFunctionTable(L, "clear", &slua_consoleClear);
   lua_setglobal(L, LLIB_CONSOLE);

   luaPushFunctionGlobal(L, "rand", &slua_rand);
   luaPushFunctionGlobal(L, "toggleStats", &slua_toggleStats);
   luaPushFunctionGlobal(L, "openEditor", &slua_openEditor);
   luaPushFunctionGlobal(L, "setPalette", &slua_setPalette);

   lua_newtable(L);
   luaPushFunctionTable(L, "clearImageCache", &slua_clearImageCache);
   luaPushFunctionTable(L, "clearSpriteCache", &slua_clearSpriteCache);
   lua_setglobal(L, LLIB_IMG);
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

int slua_openEditor(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   fsmSend(view->gameState, GameStateOpenMapEditor);
   return 0;
}


int slua_rand(lua_State *L) {
   int lower = (int)luaL_checkinteger(L, 1);
   int upper = (int)luaL_checkinteger(L, 2);
   
   lua_pushinteger(L, appRand(appGet(), lower, upper));
   return 1;
}

int slua_toggleStats(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   framerateViewerToggle(view->framerateViewer);
   return 0;
}

int slua_clearImageCache(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   imageLibraryClear(view->imageLibrary);
   return 0;
}

int slua_clearSpriteCache(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   spriteManagerClear(view->spriteManager);
   return 0;
}

int slua_setPalette(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   StringView id = NULL;
   int result = 0;

   int inType = lua_type(L, -1);

   if (inType == LUA_TLIGHTUSERDATA) {
      id = lua_touserdata(L, -1);
   }
   else if (inType == LUA_TSTRING) {
      id = stringIntern(lua_tostring(L, -1));
   }
   else {
      lua_pushliteral(L, "Failed to load palette; input identifier is invalid type.");
      lua_error(L);
   }

   assetsSetPalette(view->db, id);
   return 0;

}
