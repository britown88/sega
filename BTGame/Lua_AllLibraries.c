#include "Lua.h"

#include "WorldView.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

void luaLoadAllLibraries(lua_State *L, WorldView *view) {
   luaL_openlibs(L);
   luaPushUserDataGlobal(L, "View", view);

   luaLoadStandardLibrary(L);

   luaRequire(L, "lib");

   luaLoadUILibrary(L);
   luaLoadActorLibrary(L);
}