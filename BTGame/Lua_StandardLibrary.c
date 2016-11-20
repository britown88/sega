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

#include "DB.h"
#include "segautils/String.h"
#include "segautils/StandardVectors.h"

static int slua_consolePrint(lua_State *L);
static int slua_consoleClear(lua_State *L);
static int slua_rand(lua_State *L);
static int slua_toggleStats(lua_State *L);
static int slua_openEditor(lua_State *L);
static int slua_clearImageCache(lua_State *L);
static int slua_clearSpriteCache(lua_State *L);
static int slua_setPalette(lua_State *L);
static int slua_toggleLightMode(lua_State *L);
static int slua_intellisense(lua_State *L);




void luaLoadStandardLibrary(lua_State *L) {

   lua_newtable(L);
   luaPushFunctionTable(L, "print", &slua_consolePrint);
   luaPushFunctionTable(L, "clear", &slua_consoleClear);
   lua_setglobal(L, LLIB_CONSOLE);

   luaPushFunctionGlobal(L, "rand", &slua_rand);
   luaPushFunctionGlobal(L, "toggleStats", &slua_toggleStats);
   luaPushFunctionGlobal(L, "openEditor", &slua_openEditor);
   luaPushFunctionGlobal(L, "setPalette", &slua_setPalette);
   luaPushFunctionGlobal(L, "toggleLightMode", &slua_toggleLightMode);
   luaPushFunctionGlobal(L, "intellij", &slua_intellisense);

   lua_newtable(L);
   luaPushFunctionTable(L, "clearImageCache", &slua_clearImageCache);
   luaPushFunctionTable(L, "clearSpriteCache", &slua_clearSpriteCache);
   lua_setglobal(L, LLIB_IMG);
}

// assumes a table is at the top of the stack and attempts to push the value
// with key <name> as the next item above it
// returns false if the pushed value is not a table (or is nil)
static bool _pushTableFromTop(lua_State *L, const char *name) {
   lua_pushstring(L, name);
   return lua_gettable(L, -2) == LUA_TTABLE;
}


vec(StringPtr) *luaIntellisense(lua_State *L, const char *line){
   vec(StringPtr) *out = vecCreate(StringPtr)(&stringPtrDestroy);
   vec(StringPtr) *split = stringSplit(line, '.');

   size_t itemCount = vecSize(StringPtr)(split);
   int stack = 0;
   size_t i = 0;

   bool err = false;

   if (itemCount) {
      lua_getglobal(L, "_G"); 
      ++stack;

      //push the tables in the sequence until our searchable table is on top
      for (i = 0; i < itemCount - 1; ++i) {
         ++stack;
         if (!_pushTableFromTop(L, c_str(*vecAt(StringPtr)(split, i)))) {
            err = true;
            break;
         }
      }

      //now search
      if (!err) {
         const char *searchItem = c_str(*vecAt(StringPtr)(split, i));
         lua_pushnil(L);  // first key
         while (lua_next(L, -2) != 0) {
            // uses 'key' (at index -2) and 'value' (at index -1) 
            if (lua_isstring(L, -2) || lua_isnumber(L, -2)) {
               //we want to copy the string out to the top before calling tostring
               //because it mgiht screw up next()
               lua_pushnil(L);
               lua_copy(L, -3, -1);

               if (stringStartsWith(lua_tostring(L, -1), searchItem, false)) {
                  String *newItem = stringCreate(lua_tostring(L, -1));
                  vecPushBack(StringPtr)(out, &newItem);
               }

               lua_pop(L, 1);//pop the copy

            }

            // removes 'value'; keeps 'key' for next iteration 
            lua_pop(L, 1);
         }
      }

      //pop the stuff we added to the stack
      lua_pop(L, stack);
   }

   vecDestroy(StringPtr)(split);
   return out;
}

int slua_intellisense(lua_State *L) {
   const char *str = luaL_checkstring(L, 1);
   WorldView *view = luaGetWorldView(L);

   vec(StringPtr) *intellij = luaIntellisense(L, str);

   vecSort(StringPtr)(intellij, &stringPtrCompare);

   vecForEach(StringPtr, item, intellij, {
      consolePushLine(view->console, c_str(*item));
   });


   vecDestroy(StringPtr)(intellij);
   return 0;
}

int slua_consolePrint(lua_State *L) {
   const char *str = luaL_checkstring(L, 1);
   WorldView *view = luaGetWorldView(L);
   consolePushLine(view->console, str);
   return 0;
}

int slua_consoleClear(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   consoleClear(view->console);
   return 0;
}

int slua_openEditor(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   fsmSend(view->gameState, GameStateOpenMapEditor);
   return 0;
}

int slua_toggleLightMode(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   gridManagerToggleLightMode(view->gridManager);
   return 0;
}


int slua_rand(lua_State *L) {
   int lower = (int)luaL_checkinteger(L, 1);
   int upper = (int)luaL_checkinteger(L, 2);
   
   lua_pushinteger(L, appRand(appGet(), lower, upper));
   return 1;
}

int slua_toggleStats(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   framerateViewerToggle(view->framerateViewer);
   return 0;
}

int slua_clearImageCache(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   imageLibraryClear(view->imageLibrary);
   return 0;
}

int slua_clearSpriteCache(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   spriteManagerClear(view->spriteManager);
   return 0;
}

int slua_setPalette(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   StringView id = NULL;
   int result = 0;

   int inType = lua_type(L, -1);

   if (inType == LUA_TLIGHTUSERDATA) {
      id = lua_touserdata(L, -1);
   }
   else if (inType == LUA_TSTRING) {
      id = stringIntern(lua_tostring(L, -1));
   }
   else {
      lua_pushliteral(L, "Failed to load palette; input identifier is invalid type.");
      lua_error(L);
   }

   assetsSetPalette(view->db, id);
   return 0;

}
