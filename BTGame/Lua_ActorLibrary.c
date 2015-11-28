#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Entities/Entities.h"
#include "CoreComponents.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_actorMove(lua_State *L);
static int slua_actorMoveRelative(lua_State *L);
static int slua_actorPosition(lua_State *L);
static int slua_actorStop(lua_State *L);
static int slua_actorIsMoving(lua_State *L);

void luaActorAddGlobalActor(lua_State *L, const char *name, Entity *e) {
   if (luaL_dostring(L, "return Actor:new()")) { 
      const char* err = lua_tostring(L, -1);
      lua_error(L); 
   }
   luaPushUserDataTable(L, "entity", e);
   lua_setglobal(L, name);

   {
      char buff[32];
      sprintf(buff, "Actors:add(%s)", name);
      if (luaL_dostring(L, buff)) { lua_error(L); }
   }
   
}

void luaActorAddActor(lua_State *L, Entity *e) {
   lua_getglobal(L, "Actors");
   lua_pushliteral(L, "add");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);
   if (luaL_dostring(L, "return New(Actor)")) { lua_error(L); }
   luaPushUserDataTable(L, "entity", e);
   lua_call(L, 2, 0);
   lua_pop(L, 1);
}

void luaActorRemoveActor(lua_State *L, Entity *e) {
   lua_getglobal(L, "Actors");
   lua_pushliteral(L, "remove");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);
   if (luaL_dostring(L, "return New(Actor)")) { lua_error(L); }
   luaPushUserDataTable(L, "entity", e);
   lua_call(L, 2, 0);
   lua_pop(L, 1);
}

void luaActorStepAllScripts(lua_State *L) {
   lua_getglobal(L, "Actors");
   lua_pushliteral(L, "stepScripts");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);
   if (lua_pcall(L, 1, 0, 0)) {
      const char* err = lua_tostring(L, -1);
      lua_error(L);
   }
   lua_pop(L, 1);
}

void luaLoadActorLibrary(lua_State *L) {
   if (luaL_dofile(L, "assets/lua/lib/actor.lua")) {
      const char* err = lua_tostring(L, -1);
      lua_error(L);
   }

   lua_getglobal(L, "Actor");
   luaPushUserDataTable(L, "entity", NULL);
   luaPushFunctionTable(L, "move", &slua_actorMove);
   luaPushFunctionTable(L, "moveRelative", &slua_actorMoveRelative);
   luaPushFunctionTable(L, "position", &slua_actorPosition);
   luaPushFunctionTable(L, "stop", &slua_actorStop);
   luaPushFunctionTable(L, "isMoving", &slua_actorIsMoving);
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

int slua_actorMoveRelative(lua_State *L) {
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

   gridMovementManagerMoveEntityRelative(view->managers->gridMovementManager, e, x, y);
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

int slua_actorStop(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   Entity *e = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   e = luaGetUserDataFromTable(L, 1, "entity");

   if (!e) {
      lua_pushliteral(L, "Actor Entity not valid.");
      lua_error(L);
   }

   gridMovementManagerStopEntity(view->managers->gridMovementManager, e);

   return 0;
}


int slua_actorIsMoving(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   Entity *e = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   e = luaGetUserDataFromTable(L, 1, "entity");

   if (!e) {
      lua_pushliteral(L, "Actor Entity not valid.");
      lua_error(L);
   }

   if (gridMovementManagerEntityIsMoving(view->managers->gridMovementManager, e)) {
      lua_pushboolean(L, true);
   }
   else {
      lua_pushboolean(L, false);
   }

   return 1;
}