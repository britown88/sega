#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Entities/Entities.h"
#include "CoreComponents.h"
#include "Console.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_actorPushScript(lua_State *L);
static int slua_actorPopScript(lua_State *L);
static int slua_actorStepScript(lua_State *L);
static int slua_actorMove(lua_State *L);
static int slua_actorMoveRelative(lua_State *L);
static int slua_actorPosition(lua_State *L);
static int slua_actorStop(lua_State *L);
static int slua_actorIsMoving(lua_State *L);
static int slua_actorDistanceTo(lua_State *L);

void luaActorAddGlobalActor(lua_State *L, const char *name, Entity *e) {
   //call new
   lua_pushcfunction(L, &luaNewObject);
   lua_getglobal(L, "Actor");
   lua_call(L, 1, 1);

   luaPushUserDataTable(L, "entity", e);

   lua_pushliteral(L, "scripts");
   lua_newtable(L);
   lua_rawset(L, -3);

   lua_setglobal(L, name);

   lua_getglobal(L, "Actors");
   lua_pushliteral(L, "add");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);
   lua_getglobal(L, "Player");
   lua_call(L, 2, 0);

   lua_pop(L, 1);   
}
void luaActorAddActor(lua_State *L, Entity *e) {
   lua_getglobal(L, "Actors");
   lua_pushliteral(L, "add");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);
   
   //call new
   lua_pushcfunction(L, &luaNewObject);
   lua_getglobal(L, "Actor");
   lua_call(L, 1, 1);

   luaPushUserDataTable(L, "entity", e);

   lua_pushliteral(L, "scripts");
   lua_newtable(L);
   lua_rawset(L, -3);

   lua_call(L, 2, 0);
   lua_pop(L, 1);
}
void luaActorRemoveActor(lua_State *L, Entity *e) {
   lua_getglobal(L, "Actors");
   lua_pushliteral(L, "remove");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);

   //call new
   lua_pushcfunction(L, &luaNewObject);
   lua_getglobal(L, "Actor");
   lua_call(L, 1, 1);

   luaPushUserDataTable(L, "entity", e);
   lua_call(L, 2, 0);
   lua_pop(L, 1);
}
void luaActorStepAllScripts(WorldView *view, lua_State *L) {
   int aCount, i;
   lua_getglobal(L, "Actors");
   
   lua_len(L, -1);
   aCount = (int)luaL_checkinteger(L, -1);
   lua_pop(L, 1);
   
   for (i = 0; i < aCount; ++i) {
      lua_pushcfunction(L, &slua_actorStepScript);

      lua_pushinteger(L, i + 1);
      lua_gettable(L, -3);//push the actor

      if (lua_pcall(L, 1, 0, 0)) {
         if (lua_type(L, -1) == LUA_TSTRING) {
            const char *err = lua_tostring(L, -1);
            consolePrintLine(view->console, "[c=0,13]Error stepping script:\n[=]%s[/=][/c]", err);
         }
         else {
            consolePrintLine(view->console, "[c=0,13]Unspecified error stepping script.[/c]");
         }

         lua_pop(L, 1);
      }
   }

   lua_pop(L, 1); //pop actors
}

void luaLoadActorLibrary(lua_State *L) {
   lua_newtable(L);

   //Entity*
   luaPushUserDataTable(L, "entity", NULL);

   //add empty scripts table
   lua_pushliteral(L, "scripts");
   lua_newtable(L);
   lua_rawset(L, -3);

   luaPushFunctionTable(L, "pushScript", &slua_actorPushScript);
   luaPushFunctionTable(L, "popScript", &slua_actorPopScript);
   luaPushFunctionTable(L, "stepScript", &slua_actorStepScript);

   luaPushFunctionTable(L, "move", &slua_actorMove);
   luaPushFunctionTable(L, "moveRelative", &slua_actorMoveRelative);
   luaPushFunctionTable(L, "position", &slua_actorPosition);
   luaPushFunctionTable(L, "stop", &slua_actorStop);
   luaPushFunctionTable(L, "isMoving", &slua_actorIsMoving);
   luaPushFunctionTable(L, "distanceTo", &slua_actorDistanceTo);
   
   lua_setglobal(L, "Actor");
}

int slua_actorPushScript(lua_State *L) {
   int n = lua_gettop(L);
   int i, result, index;
   lua_State *thread;

   luaL_checktype(L, 1, LUA_TTABLE);//1 - actor, self
   luaL_checktype(L, 2, LUA_TFUNCTION);//2 - script

   //create our thread and push script, actor, ...
   thread = lua_newthread(L);// n+1: thread
   lua_pushvalue(L, 2);
   lua_pushvalue(L, 1);
   for (i = 0; i < n - 2; ++i) {
      lua_pushvalue(L, 3 + i);
   }
   lua_xmove(L, thread, n);

   //start the thread
   result = lua_resume(thread, NULL, n - 1);

   if (result != LUA_YIELD) {
      //function executed or yielded,  pop the thread and return
      lua_pop(L, 1);

      if (result != LUA_OK) {
         const char *err = luaL_checkstring(L, -1);
         lua_pushvalue(L, -1);
         lua_error(L);
      }    

      return 0;
   }

   //now store the thread in the actor's scripts
   lua_pushvalue(L, 1);//push the actor
   lua_pushliteral(L, "scripts");
   lua_gettable(L, -2);//push the actor's script table
   
   //get the len, add 1 and push it
   lua_len(L, -1);//push the len
   index = (int)lua_tointeger(L, -1) + 1;
   lua_pop(L, 1);
   lua_pushinteger(L, index);

   lua_pushvalue(L, n + 1);//push the thread
   lua_settable(L, -3);//commit script[#scripts] = thread

   lua_pop(L, 3);//pop the scripts, actor, and thread
   return 0;
}
int slua_actorPopScript(lua_State *L) {
   luaL_checktype(L, 1, LUA_TTABLE);//1 - actor, self

   lua_pushliteral(L, "scripts");
   lua_gettable(L, -2);//push the actor's script table

   lua_len(L, -1);
   if (lua_tointeger(L, -1) > 0) {
      lua_pushnil(L);//index is pushed, push nil and set it
      lua_settable(L, -3);
   }
   else {
      lua_pop(L, 1);//pop the len
   }

   lua_pop(L, 1);//pop the scripts table  
   return 0;
}
int slua_actorStepScript(lua_State *L) {
   int index;
   int n = lua_gettop(L);
   luaL_checktype(L, 1, LUA_TTABLE);//1 - actor, self

   lua_pushliteral(L, "scripts");
   lua_gettable(L, -2);//push the actor's script table

   lua_len(L, -1);
   index = (int)lua_tointeger(L, -1);

   if (index > 0) {
      int result;
      lua_gettable(L, -2);//push the thread
      result = lua_resume(lua_tothread(L, -1), NULL, 0);

      if (result != LUA_YIELD) {       

         //now we need to call table.remove ._.
         lua_getglobal(L, "table");
         lua_pushliteral(L, "remove");
         lua_gettable(L, -2);
         lua_pushvalue(L, n + 1);//push the scripts table
         lua_pushinteger(L, index);
         lua_call(L, 2, 0);//call the remove
         lua_pop(L, 1);//pop table

         if (result != LUA_OK) {
            const char *err = lua_tostring(L, -1);
            lua_pushvalue(L, -1);
            lua_error(L);
         }
      }
      lua_pop(L, 1);//pop the thread
   }
   else {
      lua_pop(L, 1);//pop the len
   }

   lua_pop(L, 1);//pop the scripts table  
   return 0;
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
int slua_actorDistanceTo(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   Entity *e = NULL;
   int x = (int)luaL_checknumber(L, 2);
   int y = (int)luaL_checknumber(L, 3);
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

   lua_pushinteger(L, gridDistance(gc->x, gc->y, x, y));
   return 1;
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