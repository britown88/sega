#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Entities/Entities.h"
#include "CoreComponents.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_actorMove(lua_State *L);
static int slua_actorPosition(lua_State *L);

void luaActorAddGlobalActor(lua_State *L, const char *name, Entity *e) {
   if (luaL_dostring(L, "return New(Actor)")) { lua_error(L); }
   luaPushUserDataTable(L, "entity", e);
   lua_setglobal(L, name);
}

void luaLoadActorLibrary(lua_State *L) {
   if (luaL_dofile(L, "assets/lua/lib/actor.lua")) {
      lua_pop(L, 1);
      return;
   }

   lua_getglobal(L, "Actor");
   luaPushUserDataTable(L, "entity", NULL);
   luaPushFunctionTable(L, "move", &slua_actorMove);
   luaPushFunctionTable(L, "position", &slua_actorPosition);
   lua_pop(L, 1);
}

int slua_actorMove(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int x = (int)luaL_checknumber(L, 2);
   int y = (int)luaL_checknumber(L, 3);
   Entity *e = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   e = luaGetUserDataFromTable(L, 1, "entity");

   if (!e) {
      lua_pushliteral(L, "Actor Entity not valid.");
      lua_error(L);
   }

   gridMovementManagerMoveEntity(view->managers->gridMovementManager, e, x, y);
   return 0;
}

int slua_actorPosition(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   Entity *e = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);
   GridComponent *gc;

   e = luaGetUserDataFromTable(L, 1, "entity");

   if (!e) {
      lua_pushliteral(L, "Actor Entity not valid.");
      lua_error(L);
   }

   gc = entityGet(GridComponent)(e);
   if (!gc) {
      lua_pushliteral(L, "Actor Entity has no Grid Component.");
      lua_error(L);
   }

   lua_pushnumber(L, gc->x);
   lua_pushnumber(L, gc->y);

   return 2;
}