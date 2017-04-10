#include "Lua.h"

#include "WorldView.h"
#include "Console.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"


void luaLoadAllLibraries(lua_State *L, WorldView *view) {
   luaL_openlibs(L);

   luaPushUserDataGlobal(L, LLIB_VIEW, view);

   luaRequire(L, "core");

   luaLoadDBLibrary(L);
  
   luaLoadActorLibrary(L);
   luaLoadStandardLibrary(L);
   luaLoadUILibrary(L);
   luaLoadMapLibrary(L);
   luaLoadTimeLibrary(L);
}

void luaLoadAssets(lua_State *L) {
   luaCreateScriptLoaders(L);
   luaRequire(L, "assets");
}

void luaStartup(lua_State *L) {
   luaRequire(L, "startup");
}

void luaBuildDB(lua_State *L) {
   luaRequire(L, "buildDB");
}