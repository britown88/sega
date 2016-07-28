#pragma once

#include "Verbs.h"
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

#define LLIB_TEXT_AREA "textArea"
#define LLIB_TEXT_AREAS "textAreas"

#define LLIB_CONSOLE "console"
#define LLIB_IMG "img"
#define LLIB_VIEW "view"
#define LLIB_ACTOR "actor"
#define LLIB_ACTORS "actors"
#define LLIB_PLAYER "player"
#define LLIB_MAP "map"
#define LLIB_PAL "pal"

void luaLoadAllLibraries(lua_State *L, WorldView *view);
void luaLoadAssets(lua_State *L);
void luaStartup(lua_State *L);

void luaLoadStandardLibrary(lua_State *L);

void luaLoadMapLibrary(lua_State *L);

void luaLoadActorLibrary(lua_State *L);
void luaActorAddActor(lua_State *L, Entity *e);//add an entity to the actors table (called by adding an ActorComponent)
void luaActorRemoveActor(lua_State *L, Entity *e);//remove an added actor from the actors table (called by removing an actorComponent)
void luaActorMakeActorGlobal(lua_State *L, Entity *e, const char *name);//make an ALREADY_ADDED actor global (ie: player)
int luaActorGetIndex(lua_State *L, Entity *e);//returns the current 1-based index of the actor in the actors table.  returns 0 for failure
int luaActorStepAllScripts(WorldView *view, lua_State *L);//calls stepScript on every loaded actor
void luaActorInteract(lua_State *L, Entity *e, Verbs v);//actor:pushScript(actor.responses.verb) if it is available

void luaLoadUILibrary(lua_State *L);
void luaUIAddTextArea(lua_State *L, StringView name);




