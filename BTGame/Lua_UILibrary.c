#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Console.h"
#include "ChoicePrompt.h"
#include "TextArea.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

#include "segautils/StandardVectors.h"


static int slua_textAreaPush(lua_State *L);
static int slua_textAreaHide(lua_State *L);
static int slua_textAreaShow(lua_State *L);
static int slua_textAreaSetVisibility(lua_State *L);
static int slua_textAreaWait(lua_State *L);

static int slua_promptChoices(lua_State *L);

static int slua_textAreaAddTextArea(lua_State *L) {
   const char *name = lua_tostring(L, 1);
   TextArea *area = lua_touserdata(L, 2);   
   
   lua_getglobal(L, LLIB_TEXT_AREAS);
   lua_pushstring(L, name);

   lua_pushcfunction(L, &luaNewObject);
   lua_getglobal(L, LLIB_TEXT_AREA);
   lua_call(L, 1, 1);

   luaPushUserDataTable(L, "area", (void*)area);

   lua_settable(L, -3);
   lua_pop(L, 1);
   return 0;
}

void luaUIAddTextArea(lua_State *L, StringView name, TextArea *area) {
   lua_pushcfunction(L, &slua_textAreaAddTextArea);
   lua_pushstring(L, name);
   lua_pushlightuserdata(L, (void*)area);
   if (lua_pcall(L, 2, 0, 0)) {
      WorldView *view = luaGetWorldView(L);
      consolePrintLuaError(view->console, "Error adding text area");
   }
   
}

void luaLoadUILibrary(lua_State *L) {
   lua_newtable(L);
   lua_setglobal(L, LLIB_TEXT_AREAS);
   
   lua_newtable(L);
   luaPushFunctionTable(L, "push", &slua_textAreaPush);
   luaPushFunctionTable(L, "hide", &slua_textAreaHide);
   luaPushFunctionTable(L, "show", &slua_textAreaShow);
   luaPushFunctionTable(L, "wait", &slua_textAreaWait);
   luaPushFunctionTable(L, "setVisibility", &slua_textAreaSetVisibility);
   lua_setglobal(L, LLIB_TEXT_AREA);

   luaPushFunctionGlobal(L, "promptChoices", &slua_promptChoices);
}

static TextArea *_resolveTextAreaFromStack(WorldView *view, lua_State *L, int idx) {
   TextArea *out = NULL;   
   luaL_checktype(L, idx, LUA_TTABLE);

   lua_pushliteral(L, "area");
   lua_gettable(L, idx);
   if (lua_isuserdata(L, -1)) {
      out = lua_touserdata(L, -1);
   }

   lua_pop(L, 1);

   if (!out) {
      lua_pushliteral(L, "Invalid text area object.");
      lua_error(L);
   }

   return out;
}

int slua_textAreaPush(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *msg = luaL_checkstring(L, 2);
   TextArea *area = _resolveTextAreaFromStack(view, L, 1);

   textAreaPushText(area, msg);

   return 0;
}

int slua_textAreaWaitK(lua_State *L, int status, lua_KContext ctx) {
   WorldView *view = luaGetWorldView(L);
   TextArea *area = lua_touserdata(L, -1);

   if (!textAreaIsDone(area)) {
      return lua_yieldk(L, 0, ctx + 1, &slua_textAreaWaitK);
   }

   return 0;
}


int slua_textAreaWait(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   TextArea *area = _resolveTextAreaFromStack(view, L, 1);

   lua_pushlightuserdata(L, (void*)area);
   return lua_yieldk(L, 1, 0, &slua_textAreaWaitK);
}


int slua_textAreaHide(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   TextArea *area = _resolveTextAreaFromStack(view, L, 1);

   textAreaHide(area);

   return 0;
}

int slua_textAreaShow(lua_State *L) {
   WorldView *view = luaGetWorldView(L);

   TextArea *area = _resolveTextAreaFromStack(view, L, 1);

   textAreaShow(area);

   return 0;
}

int slua_textAreaSetVisibility(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   TextArea *area = _resolveTextAreaFromStack(view, L, 1);
   bool show = lua_toboolean(L, 2);

   textAreaSetVisibility(area, show);

   return 0;
}

int slua_promptChoicesK(lua_State *L, int status, lua_KContext ctx) {
   WorldView *view = luaGetWorldView(L);
   const char *result = choicePromptGetDecision(view->choicePrompt);
   int selectionIndex = choicePromptGetDecisionIndex(view->choicePrompt);
   if (!result) {
      return lua_yieldk(L, 0, ctx + 1, &slua_promptChoicesK);
   }
   else {
      lua_pushinteger(L, selectionIndex);
      lua_pushstring(L, result);
      return 2;
   }
}

int slua_promptChoices(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int choiceCount = 0, i = 0;
   vec(StringPtr) *choices = vecCreate(StringPtr)(&stringPtrDestroy);

   luaL_checktype(L, 1, LUA_TTABLE);

   lua_len(L, 1);//push size of list
   choiceCount = (int)luaL_checkinteger(L, -1);
   lua_pop(L, 1);//pop the size

   for (i = 0; i < choiceCount; ++i) {
      String *choice = NULL;
      lua_pushinteger(L, i + 1);
      lua_gettable(L, 1);//push the choice off the list

      choice = stringCreate(luaL_checkstring(L, -1));
      vecPushBack(StringPtr)(choices, &choice);
      lua_pop(L, 1);//pop the choice
   }

   //just going to return the first in the list to start but
   //we will want this to yield until a UI response
   //StringPtr *choiceFrom = vecAt(StringPtr)(choices, 0);
   //lua_pushstring(L, c_str(*choiceFrom));

   choicePromptSetChoices(view->choicePrompt, choices);

   //vecDestroy(StringPtr)(choices);
   return lua_yieldk(L, 0, 0, &slua_promptChoicesK);
}