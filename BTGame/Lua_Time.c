#include "Lua.h"
#include "Console.h"

#include "WorldView.h"
#include "Managers.h"
#include "GameState.h"
#include "ImageLibrary.h"
#include "Sprites.h"
#include "SEGA/App.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

#include "AssetHelpers.h"
#include "GridManager.h"
#include "Calendar.h"

#include "DB.h"

static int slua_jumpMinutes(lua_State *L);
static int slua_jumpHours(lua_State *L);
static int slua_jumpDays(lua_State *L);
static int slua_jumpWeeks(lua_State *L);
static int slua_jumpMonths(lua_State *L);
static int slua_jumpSeasons(lua_State *L);
static int slua_jumpYears(lua_State *L);
static int slua_Pause(lua_State *L);
static int slua_Resume(lua_State *L);

void luaLoadTimeLibrary(lua_State *L) {

   lua_newtable(L);
   luaPushFunctionTable(L, "jumpMinutes", &slua_jumpMinutes);
   luaPushFunctionTable(L, "jumpHours", &slua_jumpHours);
   luaPushFunctionTable(L, "jumpDays", &slua_jumpDays);
   luaPushFunctionTable(L, "jumpWeeks", &slua_jumpWeeks);
   luaPushFunctionTable(L, "jumpMonths", &slua_jumpMonths);
   luaPushFunctionTable(L, "jumpSeasons", &slua_jumpSeasons);
   luaPushFunctionTable(L, "jumpYears", &slua_jumpYears);
   
   luaPushFunctionTable(L, "pause", &slua_Pause);
   luaPushFunctionTable(L, "resume", &slua_Resume);
   lua_setglobal(L, "time");
}

int slua_jumpMinutes(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int m = luaL_checkinteger(L, -1);

   DateTime dt = { 0 };
   dt.time.minute = m;

   calendarJump(view->calendar, dt);
   return 0;
}
int slua_jumpHours(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int h = luaL_checkinteger(L, -1);

   DateTime dt = { 0 };
   dt.time.hour = h;

   calendarJump(view->calendar, dt);
   return 0;
}
int slua_jumpDays(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int h = luaL_checkinteger(L, -1);

   DateTime dt = { 0 };
   dt.date.day = h;

   calendarJump(view->calendar, dt);
   return 0;
}
int slua_jumpWeeks(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int h = luaL_checkinteger(L, -1);

   DateTime dt = { 0 };
   dt.date.week = h;

   calendarJump(view->calendar, dt);
   return 0;
}
int slua_jumpMonths(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int h = luaL_checkinteger(L, -1);

   DateTime dt = { 0 };
   dt.date.month = h;

   calendarJump(view->calendar, dt);
   return 0;
}
int slua_jumpSeasons(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int h = luaL_checkinteger(L, -1);

   DateTime dt = { 0 };
   dt.date.season = h;

   calendarJump(view->calendar, dt);
   return 0;
}
int slua_jumpYears(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int h = luaL_checkinteger(L, -1);

   DateTime dt = { 0 };
   dt.date.year = h;

   calendarJump(view->calendar, dt);
   return 0;
}
int slua_Pause(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   calendarPause(view->calendar);
   return 0;
}
int slua_Resume(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   calendarResume(view->calendar);
   return 0;
}


