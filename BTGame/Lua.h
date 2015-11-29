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

void *luaGetUserDataFromTable(lua_State *L, int tableIndex, const char *index);

int luaNewObject(lua_State *L);

void luaRequire(lua_State *L, const char *modname);

//library functions
void luaLoadAllLibraries(lua_State *L, WorldView *view);

void luaLoadStandardLibrary(lua_State *L);

void luaLoadActorLibrary(lua_State *L);
void luaActorAddActor(lua_State *L, Entity *e);//add an entity to the actors table (called by adding an ActorComponent)
void luaActorRemoveActor(lua_State *L, Entity *e);//remove an added actor from the actors table (called by removing an actorComponent)
void luaActorMakeActorGlobal(lua_State *L, Entity *e, const char *name);//make an ALREADY_ADDED actor global (ie: player)
void luaActorPushActor(lua_State *L, Entity *e);//push the corresponding actor table to the stack, pushes nil if it doesnt exist
void luaActorStepAllScripts(WorldView *view, lua_State *L);//calls stepScript on every loaded actor

void luaLoadUILibrary(lua_State *L);
void luaUIAddTextArea(lua_State *L, StringView name);




