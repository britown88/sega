#pragma once

#include "segashared/Strings.h"

typedef struct Entity_t Entity;
typedef struct lua_State lua_State;
typedef int(*lua_CFunction) (lua_State *L);
typedef struct WorldView_t WorldView;

lua_State *luaCreate();
void luaDestroy(lua_State *L);

WorldView *luaGetWorldView(lua_State *L);

void luaPushFunctionGlobal(lua_State *L, const char *name, lua_CFunction func);
void luaPushFunctionTable(lua_State *L, const char *index, lua_CFunction func);

void luaPushUserDataGlobal(lua_State *L, const char *name, void *data);
void luaPushUserDataTable(lua_State *L, const char *index, void *data);

int luaNewObject(lua_State *L);

//library functions
void luaLoadAllLibraries(lua_State *L, WorldView *view);

void luaLoadStandardLibrary(lua_State *L);

void luaLoadActorLibrary(lua_State *L);
void luaActorAddGlobalActor(lua_State *L, const char *name, Entity *e);

void luaLoadUILibrary(lua_State *L);
void luaUIAddTextArea(lua_State *L, StringView name);




