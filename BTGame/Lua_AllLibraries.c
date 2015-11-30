#include "Lua.h"

#include "WorldView.h"
#include "Console.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

int slua_panic(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   consolePrintLuaError(view->console, "Error creating actor");
   return 0;
}

void luaLoadAllLibraries(lua_State *L, WorldView *view) {
   luaL_openlibs(L);
   //lua_atpanic(L, &slua_panic);
   luaPushUserDataGlobal(L, LLIB_VIEW, view);
   luaRequire(L, "core");
  
   luaLoadActorLibrary(L);
   luaLoadStandardLibrary(L);
   luaLoadUILibrary(L);

}