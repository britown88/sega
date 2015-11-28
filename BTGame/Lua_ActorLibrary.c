#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Entities/Entities.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_actorMove(lua_State *L);

void luaActorAddGlobalActor(lua_State *L, const char *name, Entity *e) {
   //push the class
   lua_getglobal(L, "Actor"); // 1

   lua_pushliteral(L, "new");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);

   lua_call(L, 1, 1);

   //set the name
   lua_pushliteral(L, "entity");
   lua_pushlightuserdata(L, e);
   lua_rawset(L, -3);

   lua_setglobal(L, name);

   //pop the class and collection
   lua_pop(L, 1);
}

void luaLoadActorLibrary(lua_State *L) {
   lua_newtable(L);

   lua_pushliteral(L, "entity");
   lua_pushlightuserdata(L, NULL);
   lua_rawset(L, -3);

   luaPushFunctionTable(L, "new", &luaNewObject);
   luaPushFunctionTable(L, "move", &slua_actorMove);

   lua_setglobal(L, "Actor");
}

int slua_actorMove(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int x = (int)luaL_checknumber(L, 2);
   int y = (int)luaL_checknumber(L, 3);
   Entity *e = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   lua_pushliteral(L, "entity");
   lua_gettable(L, 1);
   if (lua_isuserdata(L, -1)) {
      e = lua_touserdata(L, -1);
   }
   lua_pop(L, 1);

   if (!e) {
      lua_pushliteral(L, "Actor Entity not valid.");
      lua_error(L);
   }

   gridMovementManagerMoveEntity(view->managers->gridMovementManager, e, x, y);
   return 0;
}