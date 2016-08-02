#include "Lua.h"
#include "Console.h"

#include "WorldView.h"
#include "Managers.h"
#include "GameState.h"
#include "ImageLibrary.h"
#include "SEGA/App.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_consolePrint(lua_State *L);
static int slua_consoleClear(lua_State *L);
static int slua_rand(lua_State *L);
static int slua_toggleStats(lua_State *L);
static int slua_openEditor(lua_State *L);
static int slua_clearCache(lua_State *L);


void luaLoadStandardLibrary(lua_State *L) {

   lua_newtable(L);
   luaPushFunctionTable(L, "print", &slua_consolePrint);
   luaPushFunctionTable(L, "clear", &slua_consoleClear);
   lua_setglobal(L, LLIB_CONSOLE);

   luaPushFunctionGlobal(L, "rand", &slua_rand);
   luaPushFunctionGlobal(L, "toggleStats", &slua_toggleStats);
   luaPushFunctionGlobal(L, "openEditor", &slua_openEditor);

   lua_newtable(L);
   luaPushFunctionTable(L, "clearCache", &slua_clearCache);
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
   renderManagerToggleFPS(view->managers->renderManager);
   return 0;
}

int slua_clearCache(lua_State *L) {
   WorldView *view = luaGetWorldView(L);

   imageLibraryClear(view->imageLibrary);

   return 0;
}