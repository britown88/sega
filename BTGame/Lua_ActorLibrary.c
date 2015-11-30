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

//add an entity to the actors table (called by adding an ActorComponent)
void luaActorAddActor(lua_State *L, Entity *e) {
   lua_getglobal(L, LLIB_ACTORS);
   lua_pushliteral(L, "add");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);

   //call new
   lua_pushcfunction(L, &luaNewObject);
   lua_getglobal(L, LLIB_ACTOR);
   lua_call(L, 1, 1);

   luaPushUserDataTable(L, "entity", e);

   lua_pushliteral(L, "scripts");
   lua_newtable(L);
   lua_rawset(L, -3);

   lua_call(L, 2, 0);
   lua_pop(L, 1);
}

//remove an added actor from the actors table (called by removing an actorComponent)
void luaActorRemoveActor(lua_State *L, Entity *e) {
   lua_getglobal(L, LLIB_ACTORS);
   lua_pushliteral(L, "remove");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);

   lua_newtable(L);
   luaPushUserDataTable(L, "entity", e);
   lua_call(L, 2, 0);

   lua_pop(L, 1);
}

//make an ALREADY_ADDED actor global (ie: player)
void luaActorMakeActorGlobal(lua_State *L, Entity *e, const char *name) {
   int len = lua_gettop(L);
   int len2;
   luaActorPushActor(L, e);
   len2 = lua_gettop(L);

   if (!lua_isnil(L, -1)) {
      lua_setglobal(L, name);
   }
   else {
      lua_pop(L, 1);
   }
}

//push the corresponding actor table to the stack, pushes nil if it doesnt exist
void luaActorPushActor(lua_State *L, Entity *e) {
   lua_getglobal(L, LLIB_ACTORS);//push actors
   lua_pushliteral(L, "get");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);

   lua_newtable(L);
   luaPushUserDataTable(L, "entity", e);
   lua_call(L, 2, 1);//push the actor

   lua_remove(L, -2);//remove the Actors table
}

//returns the current 1-based index of the actor in the actors table.  returns 0 for failure
int luaActorGetIndex(lua_State *L, Entity *e) {
   int out;
   lua_getglobal(L, LLIB_ACTORS);//push actors
   lua_pushliteral(L, "indexOf");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);

   lua_newtable(L);
   luaPushUserDataTable(L, "entity", e);
   if (lua_pcall(L, 2, 1, 0)) {//push the index
      return 0;
   }

   out = (int)luaL_checkinteger(L, -1);
   lua_pop(L, 2);//remove the Actors table and index
   return out;
}

//calls stepScript on every loaded actor
void luaActorStepAllScripts(WorldView *view, lua_State *L) {
   int aCount, i;
   lua_getglobal(L, LLIB_ACTORS);

   lua_len(L, -1);
   aCount = (int)luaL_checkinteger(L, -1);
   lua_pop(L, 1);

   for (i = 0; i < aCount; ++i) {
      lua_pushcfunction(L, &slua_actorStepScript);

      lua_pushinteger(L, i + 1);
      lua_gettable(L, -3);//push the actor

      if (lua_pcall(L, 1, 0, 0)) {
         const char *err = lua_tostring(L, -1);
         consolePrintLine(view->console, "[c=0,13]Error stepping script:\n [=]%s[/=][/c]", err);


         lua_pop(L, 1);
      }
   }

   lua_pop(L, 1); //pop actors
}

void luaActorAddGlobalActor(lua_State *L, const char *name, Entity *e) {
   //call new
   lua_pushcfunction(L, &luaNewObject);
   lua_getglobal(L, LLIB_ACTOR);
   lua_call(L, 1, 1);

   luaPushUserDataTable(L, "entity", e);

   lua_pushliteral(L, "scripts");
   lua_newtable(L);
   lua_rawset(L, -3);

   lua_setglobal(L, name);

   lua_getglobal(L, LLIB_ACTORS);
   lua_pushliteral(L, "add");
   lua_gettable(L, -2);
   lua_pushvalue(L, -2);
   lua_getglobal(L, LLIB_PLAYER);
   lua_call(L, 2, 0);

   lua_pop(L, 1);   
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