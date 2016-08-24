#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Console.h"
#include "GridManager.h"
#include "Actors.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_actorPushScript(lua_State *L);
static int slua_actorPopScript(lua_State *L);
static int slua_actorStepScript(lua_State *L);
static int slua_actorMove(lua_State *L);
static int slua_actorTeleport(lua_State *L);
static int slua_actorMoveRelative(lua_State *L);
static int slua_actorPosition(lua_State *L);
static int slua_actorStop(lua_State *L);
static int slua_actorIsMoving(lua_State *L);
static int slua_actorDistanceTo(lua_State *L);
static int slua_actorMoveSpeed(lua_State *L);
static int slua_actorSetMoveSpeed(lua_State *L);

//C API FUNCTIONS
static int slua_actorAddActor(lua_State *L) {
   Actor *a = lua_touserdata(L, 1);
   lua_getglobal(L, LLIB_ACTORS);
   lua_pushliteral(L, "add");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);

   //call new
   lua_pushcfunction(L, &luaNewObject);
   lua_getglobal(L, LLIB_ACTOR);
   lua_call(L, 1, 1);

   luaPushUserDataTable(L, "actor", a);

   lua_pushliteral(L, "scripts");
   lua_newtable(L);
   lua_settable(L, -3);

   lua_pushliteral(L, "response");
   lua_newtable(L);
   lua_settable(L, -3);

   lua_call(L, 2, 0);

   lua_pop(L, 1);

   return 0;
}
static int slua_actorRemoveActor(lua_State *L) {
   Actor *a = lua_touserdata(L, 1);
   lua_getglobal(L, LLIB_ACTORS);
   lua_pushliteral(L, "remove");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);

   lua_newtable(L);
   luaPushUserDataTable(L, "actor", a);
   lua_call(L, 2, 0);

   lua_pop(L, 1);
   return 0;
}
static int slua_actorPushActor(lua_State *L) {
   Actor *a = lua_touserdata(L, 1);
   lua_getglobal(L, LLIB_ACTORS);//push actors
   lua_pushliteral(L, "get");
   lua_gettable(L, -2);//push get func
   lua_pushvalue(L, -2);//copy table to top

   lua_newtable(L);//add new actor
   luaPushUserDataTable(L, "actor", a);//set its udata
   lua_call(L, 2, 1);
   lua_remove(L, -2);//remove the Actors table
   return 1;
}
static int slua_actorMakeActorGlobal(lua_State *L) {
   Actor *a = lua_touserdata(L, 1);
   const char *name = lua_tostring(L, 2);

   lua_pushcfunction(L, &slua_actorPushActor);
   lua_pushvalue(L, 1);//copy the entiry
   lua_call(L, 1, 1);

   if (lua_type(L, -1) == LUA_TTABLE) {
      lua_setglobal(L, name);
   }
   else {
      lua_pop(L, 1);
      lua_pushliteral(L, "Attempted to make an Actor global that is not in the actors table.");
      lua_error(L);
   }
   return 0;
}
static int slua_actorGetIndex(lua_State *L) {
   Actor *a = lua_touserdata(L, 1);
   lua_getglobal(L, LLIB_ACTORS);//push actors
   lua_pushliteral(L, "indexOf");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);

   lua_newtable(L);
   luaPushUserDataTable(L, "actor", a);
   lua_call(L, 2, 1);

   lua_remove(L, -2);//remove the Actors table

   return 1;
}
static int slua_actorStepAllScripts(lua_State *L) {
   int aCount, i;
   lua_getglobal(L, LLIB_ACTORS);

   lua_len(L, -1);
   aCount = (int)luaL_checkinteger(L, -1);
   lua_pop(L, 1);

   for (i = 0; i < aCount; ++i) {
      lua_pushcfunction(L, &slua_actorStepScript);

      lua_pushinteger(L, i + 1);
      lua_gettable(L, -3);//push the actor

      lua_call(L, 1, 0);
   }

   lua_pop(L, 1); //pop actors
   return 0;
}
static int slua_actorInteract(lua_State *L) {
   Actor *a = lua_touserdata(L, 1); //1, the actor
   Verbs v = (Verbs)lua_tointeger(L, 2); // 2, the v

   lua_pushcfunction(L, &slua_actorPushActor);
   lua_pushvalue(L, 1);//copy the actor
   lua_call(L, 1, 1);
   //3, the actor

   lua_pushliteral(L, "response");
   lua_gettable(L, -2);//4: the  responses table

   if (lua_type(L, -1) != LUA_TTABLE) {
      lua_pop(L, 2);
      return 0;//no responses table
   }

   switch (v) {
   case Verb_Look:
      lua_pushliteral(L, "toLook");
      break;
   case Verb_Talk:
      lua_pushliteral(L, "toTalk");
      break;
   case Verb_Use:
      lua_pushliteral(L, "toUse");
      break;
   case Verb_Fight:
      lua_pushliteral(L, "toFight");
      break;
   }

   lua_gettable(L, -2);//5: the response function
   if (lua_type(L, -1) != LUA_TFUNCTION) {
      lua_pop(L, 3);
      return 0;//does not have this response
   }

   lua_pushcfunction(L, &slua_actorPushScript);//push the pushscript function
   lua_pushvalue(L, 3);//copy the actor to the front
   lua_pushvalue(L, 5);//copy the funciton to the top
   lua_call(L, 2, 0);
   lua_pop(L, 3);//actor, response table, and response function
   return 0;
}

//add an actor to the actor table (done on actor creation)
void luaActorAddActor(lua_State *L, Actor *a) {
   lua_pushcfunction(L, &slua_actorAddActor);
   lua_pushlightuserdata(L, a);
   if (lua_pcall(L, 1, 0, 0)) {
      WorldView *view = luaGetWorldView(L);
      consolePrintLuaError(view->console, "Error adding actor");
   }
}

//remove an added actor from the actors table (called by removing an actorComponent)
void luaActorRemoveActor(lua_State *L, Actor *a) {
   lua_pushcfunction(L, &slua_actorRemoveActor);
   lua_pushlightuserdata(L, a);
   if (lua_pcall(L, 1, 0, 0)) {
      WorldView *view = luaGetWorldView(L);
      consolePrintLuaError(view->console, "Error removing actor");
   }
}

//make an ALREADY_ADDED actor global (ie: player)
void luaActorMakeActorGlobal(lua_State *L, Actor *a, const char *name) {
   lua_pushcfunction(L, &slua_actorMakeActorGlobal);
   lua_pushlightuserdata(L, a);
   lua_pushstring(L, name);
   if (lua_pcall(L, 2, 0, 0)) {
      WorldView *view = luaGetWorldView(L);
      consolePrintLuaError(view->console, "Error making actor global");
   }
}

//returns the current 1-based index of the actor in the actors table.  returns 0 for failure
int luaActorGetIndex(lua_State *L, Actor *a) {
   int out = 0;
   lua_pushcfunction(L, &slua_actorGetIndex);
   lua_pushlightuserdata(L, a);
   if (lua_pcall(L, 1, 1, 0)) {
      WorldView *view = luaGetWorldView(L);
      consolePrintLuaError(view->console, "Error retreiving actor index");
   }
   else
   {
      if (lua_isinteger(L, -1)) {
         out = (int)lua_tointeger(L, -1);
      }
      lua_pop(L, 1);
   }

   return out;
}

//calls stepScript on every loaded actor
int luaActorStepAllScripts(WorldView *view, lua_State *L) {
   lua_pushcfunction(L, &slua_actorStepAllScripts);
   return lua_pcall(L, 0, 0, 0);
}

void luaActorInteract(lua_State *L, Actor *a, Verbs v) {
   lua_pushcfunction(L, &slua_actorInteract);
   lua_pushlightuserdata(L, a);
   lua_pushinteger(L, (int)v);
   if (lua_pcall(L, 2, 0, 0)) {
      WorldView *view = luaGetWorldView(L);
      consolePrintLuaError(view->console, "Error interacting");
   }
}

void luaLoadActorLibrary(lua_State *L) {
   lua_newtable(L);

   //Actor*
   luaPushUserDataTable(L, "actor", NULL);

   //add empty scripts table
   lua_pushliteral(L, "scripts");
   lua_newtable(L);
   lua_settable(L, -3);

   luaPushFunctionTable(L, "pushScript", &slua_actorPushScript);
   luaPushFunctionTable(L, "popScript", &slua_actorPopScript);
   luaPushFunctionTable(L, "stepScript", &slua_actorStepScript);

   luaPushFunctionTable(L, "move", &slua_actorMove);
   luaPushFunctionTable(L, "teleport", &slua_actorTeleport);
   luaPushFunctionTable(L, "moveRelative", &slua_actorMoveRelative);
   luaPushFunctionTable(L, "position", &slua_actorPosition);
   luaPushFunctionTable(L, "stop", &slua_actorStop);
   luaPushFunctionTable(L, "isMoving", &slua_actorIsMoving);
   luaPushFunctionTable(L, "distanceTo", &slua_actorDistanceTo);
   luaPushFunctionTable(L, "moveSpeed", &slua_actorMoveSpeed);
   luaPushFunctionTable(L, "setMoveSpeed", &slua_actorSetMoveSpeed);
   
   lua_setglobal(L, LLIB_ACTOR);
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
      //function error'd or finished,  pop the thread and return      

      if (result != LUA_OK) {
         lua_xmove(thread, L, 1);//xfer the error to the main thread
         lua_error(L);
      }    

      lua_pop(L, 1);

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
      lua_State *thread;
      lua_gettable(L, -2);//push the thread

      thread = lua_tothread(L, -1);
      result = lua_resume(thread, NULL, 0);

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
            lua_xmove(thread, L, 1);
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
   Actor *a = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   a = luaGetUserDataFromTable(L, 1, "actor");

   if (!a) {
      lua_pushliteral(L, "Actor not valid.");
      lua_error(L);
   }

   gridMovementManagerMoveActor(view->gridMovementManager, a, x, y);
   return 0;
}
int slua_actorTeleport(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int x = (int)luaL_checknumber(L, 2);
   int y = (int)luaL_checknumber(L, 3);
   Actor *a = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   a = luaGetUserDataFromTable(L, 1, "actor");

   if (!a) {
      lua_pushliteral(L, "Actor not valid.");
      lua_error(L);
   }

   actorSetGridPosition(a, (Int2) { x, y });
   actorSnap(a);

   return 0;
}
int slua_actorMoveRelative(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int x = (int)luaL_checknumber(L, 2);
   int y = (int)luaL_checknumber(L, 3);
   Actor *a = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   a = luaGetUserDataFromTable(L, 1, "actor");

   if (!a) {
      lua_pushliteral(L, "Actor not valid.");
      lua_error(L);
   }

   gridMovementManagerMoveActorRelative(view->gridMovementManager, a, x, y);
   return 0;
}
int slua_actorDistanceTo(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   Actor *a = NULL;
   int x = (int)luaL_checknumber(L, 2);
   int y = (int)luaL_checknumber(L, 3);
   luaL_checktype(L, 1, LUA_TTABLE);
   Int2 aPos;

   a = luaGetUserDataFromTable(L, 1, "actor");

   if (!a) {
      lua_pushliteral(L, "Actor not valid.");
      lua_error(L);
   }

   aPos = actorGetGridPosition(a);

   lua_pushinteger(L, gridDistance(aPos.x, aPos.y, x, y));
   return 1;
}
int slua_actorPosition(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   Actor *a = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);
   Int2 aPos;

   a = luaGetUserDataFromTable(L, 1, "actor");

   if (!a) {
      lua_pushliteral(L, "Actor not valid.");
      lua_error(L);
   }

   aPos = actorGetGridPosition(a);

   lua_pushnumber(L, aPos.x);
   lua_pushnumber(L, aPos.y);

   return 2;
}
int slua_actorStop(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   Actor *a = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   a = luaGetUserDataFromTable(L, 1, "actor");

   if (!a) {
      lua_pushliteral(L, "Actor not valid.");
      lua_error(L);
   }

   gridMovementManagerStopActor(view->gridMovementManager, a);

   return 0;
}
int slua_actorIsMoving(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   Actor *a = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   a = luaGetUserDataFromTable(L, 1, "actor");

   if (!a) {
      lua_pushliteral(L, "Actor not valid.");
      lua_error(L);
   }

   if (gridMovementManagerActorIsMoving(view->gridMovementManager, a)) {
      lua_pushboolean(L, true);
   }
   else {
      lua_pushboolean(L, false);
   }

   return 1;
}
int slua_actorMoveSpeed(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   Actor *a = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);

   a = luaGetUserDataFromTable(L, 1, "actor");

   if (!a) {
      lua_pushliteral(L, "Actor not valid.");
      lua_error(L);
   }

   lua_pushinteger(L, actorGetMoveTime(a));
   lua_pushinteger(L, actorGetMoveDelay(a));
   return 2;
}
int slua_actorSetMoveSpeed(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   Actor *a = NULL;
   luaL_checktype(L, 1, LUA_TTABLE);
   Milliseconds moveTime, moveDelay;
   bool hasDelay = false, hasTime = false;

   //parameters is a table, try to grab the individual aprts
   if (lua_type(L, 2) == LUA_TTABLE) {
      lua_pushliteral(L, "time");
      lua_gettable(L, -2);
      if (lua_isinteger(L, -1)) {
         moveTime = lua_tointeger(L, -1);
         hasTime = true;
      }
      lua_pop(L, 1);

      lua_pushliteral(L, "delay");
      lua_gettable(L, -2);
      if (lua_isinteger(L, -1)) {
         moveDelay = lua_tointeger(L, -1);
         hasDelay = true;
      }
      lua_pop(L, 1);
   }
   else {
      //expect two integers
      if (lua_isinteger(L, 2)) {
         moveTime = lua_tointeger(L, 2);
         hasTime = true;
      }
      if (lua_isinteger(L, 3)) {
         moveDelay = lua_tointeger(L, 3);
         hasDelay = true;
      }
   }

   if (!hasDelay && !hasTime) {
      return 0;
   }

   a = luaGetUserDataFromTable(L, 1, "actor");

   if (!a) {
      lua_pushliteral(L, "Actor not valid.");
      lua_error(L);
   }

   if (hasTime) {
      actorSetMoveTime(a, moveTime);
   }

   if (hasDelay) {
      actorSetMoveTime(a, moveDelay);
   }

   return 0;
}