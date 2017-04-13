#include "Lua.h"
#include "Console.h"
#include "DB.h"

#include "WorldView.h"
#include "Managers.h"
#include "GameState.h"
#include "ImageLibrary.h"
#include "assets.h"
#include "segautils/BitBuffer.h"
#include "SEGA/App.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"


static int slua_DBInsertImage(lua_State *L);
static int slua_DBInsertImageFolder(lua_State *L);
static int slua_DBInsertPalette(lua_State *L);
static int slua_DBInsertPaletteFolder(lua_State *L);
static int slua_DBInsertSprite(lua_State *L);
static int slua_DBInsertTileSchema(lua_State *L);
static int slua_DBInsertMap(lua_State *L);
static int slua_DBBegin(lua_State *L);
static int slua_DBEnd(lua_State *L);

static int slua_DBInsertLuaScript(lua_State *L);

void luaLoadDBLibrary(lua_State *L) {

   lua_newtable(L);
   luaPushFunctionTable(L, "insertImage", &slua_DBInsertImage);
   luaPushFunctionTable(L, "insertImageFolder", &slua_DBInsertImageFolder);
   luaPushFunctionTable(L, "insertPalette", &slua_DBInsertPalette);
   luaPushFunctionTable(L, "insertPaletteFolder", &slua_DBInsertPaletteFolder);
   luaPushFunctionTable(L, "insertSprite", &slua_DBInsertSprite);
   luaPushFunctionTable(L, "insertTileSchema", &slua_DBInsertTileSchema);
   luaPushFunctionTable(L, "insertLuaScript", &slua_DBInsertLuaScript);
   luaPushFunctionTable(L, "insertMap", &slua_DBInsertMap);

   luaPushFunctionTable(L, "beginTransaction", &slua_DBBegin);
   luaPushFunctionTable(L, "endTransaction", &slua_DBEnd);

   lua_setglobal(L, LLIB_DB);

}

int slua_DBBegin(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   if (dbExecute((DBBase*)view->db, "BEGIN TRANSACTION;") != DB_SUCCESS) {
      lua_pushstring(L, dbGetError((DBBase*)view->db));
      lua_error(L);
   }
   return 0;
}
int slua_DBEnd(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   if (dbExecute((DBBase*)view->db, "END TRANSACTION;") != DB_SUCCESS) {
      lua_pushstring(L, dbGetError((DBBase*)view->db));
      lua_error(L);
   }
   return 0;
}

int slua_DBInsertImageFolder(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *path = lua_tostring(L, 1);
   vec(StringPtr) *list = NULL;   
   int r = appListFiles(path, APP_FILE_FILE_ONLY, &list, "ega");

   consolePrintLine(view->console, "Inserting (*.ega) images in [c=0,5]%s[/c]", path);

   if (!r){
      vecForEach(StringPtr, str, list, {
         String *fnameOnly = stringGetFilename(*str);
         lua_pushcfunction(L, &slua_DBInsertImage);
         lua_pushstring(L, c_str(fnameOnly));
         lua_pushstring(L, c_str(*str));
         
         lua_call(L, 2, 0);
         stringDestroy(fnameOnly);
      });

      consolePrintLine(view->console, "Inserted [c=0,5]%i[/c] images.", vecSize(StringPtr)(list));
      vecDestroy(StringPtr)(list);
   }   
   else {
      consolePrintLine(view->console, "Inserted [c=0,5]%i[/c] images.", 0);
   }
   
   
   return 0;
}

int slua_DBInsertImage(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *id = lua_tostring(L, 1);
   const char *path = lua_tostring(L, 2);
   StringView interned = stringIntern(id);

   DBImage img = {.id = stringCreate(interned) };
   if (!(img.image = readFullFile(path, &img.imageSize))) {      
      dbImageDestroy(&img);
      lua_pushstring(L, "Unable to open image file");
      lua_error(L);
   }


   if (dbImageInsert(view->db, &img)) {
      dbImageDestroy(&img);
      lua_pushstring(L, dbGetError((DBBase*)view->db));
      lua_error(L);
   }

   dbImageDestroy(&img);
   consolePrintLine(view->console, "Image [c=0,5]%s[/c] inserted.", id);
   return 0;
}

int slua_DBInsertLuaScript(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *module = lua_tostring(L, 1);
   const char *path = lua_tostring(L, 2);

   DBLuaScript script = { .module = stringCreate(module) };

   byte *file;
   long fSize;

   if (!(file = readFullFile(path, &fSize))) {
      dbLuaScriptDestroy(&script);
      lua_pushstring(L, "Unable to open script file");
      lua_error(L);
   }

   script.script = stringCreate(file);

   if (dbLuaScriptInsert(view->db, &script)) {
      dbLuaScriptDestroy(&script);
      lua_pushstring(L, dbGetError((DBBase*)view->db));
      lua_error(L);
   }

   dbLuaScriptDestroy(&script);
   consolePrintLine(view->console, "Script [c=0,5]%s[/c] inserted.", module);
   return 0;
}

int slua_DBInsertMap(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *id = lua_tostring(L, 1);
   const char *path = lua_tostring(L, 2);

   DBMap map = { .id = stringCreate(id) };

   if (!(map.map = readFullFile(path, &map.mapSize))) {
      dbMapDestroy(&map);
      lua_pushstring(L, "Unable to open map file");
      lua_error(L);
   }


   if (dbMapInsert(view->db, &map)) {
      dbMapDestroy(&map);
      lua_pushstring(L, dbGetError((DBBase*)view->db));
      lua_error(L);
   }

   dbMapDestroy(&map);
   consolePrintLine(view->console, "Map [c=0,5]%s[/c] inserted.", id);
   return 0;
}


int slua_DBInsertPalette(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *id = lua_tostring(L, 1);
   const char *path = lua_tostring(L, 2);
   StringView interned = stringIntern(id);
   DBPalette pal = { .id = stringCreate(interned) };
   Palette p = paletteDeserialize(path);

   if (!memcmp(&p, &(Palette){0}, sizeof(Palette))) {
      dbPaletteDestroy(&pal);
      lua_pushstring(L, "Failed to open palette file");
      lua_error(L);
   }

   pal.palette = &p;
   pal.paletteSize = sizeof(Palette);

   if (dbPaletteInsert(view->db, &pal)) {
      dbPaletteDestroy(&pal);
      lua_pushstring(L, dbGetError((DBBase*)view->db));
      lua_error(L);
   }

   pal.palette = NULL;//dont want destory to free our stack-palette
   dbPaletteDestroy(&pal);

   consolePrintLine(view->console, "Palette [c=0,5]%s[/c] inserted.", id);
   return 0;
}

int slua_DBInsertPaletteFolder(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *path = lua_tostring(L, 1);
   vec(StringPtr) *list = NULL;
   int r = appListFiles(path, APP_FILE_FILE_ONLY, &list, "pal");

   consolePrintLine(view->console, "Inserting (*.pal) palettes in [c=0,5]%s[/c]", path);

   if (!r) {
      vecForEach(StringPtr, str, list, {
         String *fnameOnly = stringGetFilename(*str);
         lua_pushcfunction(L, &slua_DBInsertPalette);
         lua_pushstring(L, c_str(fnameOnly));
         lua_pushstring(L, c_str(*str));

         lua_call(L, 2, 0);
         stringDestroy(fnameOnly);
      });

      consolePrintLine(view->console, "Inserted [c=0,5]%i[/c] palette.", vecSize(StringPtr)(list));
      vecDestroy(StringPtr)(list);
   }
   else {
      consolePrintLine(view->console, "Inserted [c=0,5]%i[/c] palette.", 0);
   }


   return 0;
}

int slua_DBInsertSprite(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   DBSprite newSprite = { 0 };
   String *lastImage = stringCreate("");
   int fCount = 0, i = 0;

   luaL_checktype(L, -1, LUA_TTABLE);

   lua_pushliteral(L, "id");
   lua_gettable(L, -2);//push id
   newSprite.id = stringCreate(luaL_checkstring(L, -1));
   lua_pop(L, 1);//pop id

   lua_pushliteral(L, "size");
   lua_gettable(L, -2);//push size
   luaL_checktype(L, -1, LUA_TTABLE);

   //check the size
   lua_len(L, -1);//push len
   if (luaL_checkinteger(L, -1) != 2) {
      lua_pushliteral(L, "Size must contain two integers, x then y.");
      lua_error(L);
   }
   lua_pop(L, 1);//pop the len

   //width
   lua_pushinteger(L, 1);
   lua_gettable(L, -2);//push width
   newSprite.width = (int)luaL_checkinteger(L, -1);
   lua_pop(L, 1);//pop width

   //height
   lua_pushinteger(L, 2);
   lua_gettable(L, -2);//push height
   newSprite.height = (int)luaL_checkinteger(L, -1);
   lua_pop(L, 1);//pop height

   lua_pop(L, 1);//pop the size table
   
   //sprites done now we need the frames
   //first we're gonna delete any frames under this name that may already be there
   dbSpriteFrameDeleteBysprite(view->db, c_str(newSprite.id));

   //push frames table and get size
   lua_pushliteral(L, "frames");
   lua_gettable(L, -2);//push frames table
   luaL_checktype(L, -1, LUA_TTABLE);
   lua_len(L, -1);//push len
   fCount = (int)luaL_checkinteger(L, -1);
   lua_pop(L, 1);//pop the len

   for (i = 0; i < fCount; ++i) {
      DBSpriteFrame newFrame = { 0 };
      lua_pushinteger(L, i + 1);
      lua_gettable(L, -2);//push currnet frame
      luaL_checktype(L, -1, LUA_TTABLE);

      newFrame.index = i;
      newFrame.sprite = stringCopy(newSprite.id);

      lua_pushliteral(L, "image");
      lua_gettable(L, -2);//push img
      if (lua_isnil(L, -1)) {
         //no image means we should use the last image used
         if (i == 0) {
            lua_pushliteral(L, "Frame must have an image or else a previous entry that defines one");
            lua_error(L);
         }

         newFrame.image = stringCopy(lastImage);
      }
      else {
         newFrame.image = stringCreate(luaL_checkstring(L, -1));
         stringSet(lastImage, c_str(newFrame.image));
      }
      
      lua_pop(L, 1);//pop img

      lua_pushliteral(L, "pos");
      lua_gettable(L, -2);//push pos
      luaL_checktype(L, -1, LUA_TTABLE);

      //check the size
      lua_len(L, -1);//push len
      if (luaL_checkinteger(L, -1) != 2) {
         lua_pushliteral(L, "Pos must contain two integers, x then y.");
         lua_error(L);
      }
      lua_pop(L, 1);//pop the len

      //x
      lua_pushinteger(L, 1);
      lua_gettable(L, -2);//push x
      newFrame.imgX = (int)luaL_checkinteger(L, -1);
      lua_pop(L, 1);//pop x

      //y
      lua_pushinteger(L, 2);
      lua_gettable(L, -2);//push y
      newFrame.imgY = (int)luaL_checkinteger(L, -1);
      lua_pop(L, 1);//pop y

      lua_pop(L, 1);//pop the pos table
      lua_pop(L, 1);//pop the current frame
      dbSpriteFrameInsert(view->db, &newFrame);
      dbSpriteFrameDestroy(&newFrame);
   }

   lua_pop(L, 1);//pop the frames table

   dbSpriteInsert(view->db, &newSprite);
   dbSpriteDestroy(&newSprite);

   stringDestroy(lastImage);
   return 0;
}

int slua_DBInsertTileSchema(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   String *setName = NULL;
   int count = 0, i = 0;

   luaL_checktype(L, -1, LUA_TTABLE);

   lua_pushliteral(L, "set");
   lua_gettable(L, -2);//push setname
   setName = stringCreate(luaL_checkstring(L, -1));
   lua_pop(L, 1);//pop setname

   //push tiles table and get size
   lua_pushliteral(L, "tiles");
   lua_gettable(L, -2);//push tiles table
   luaL_checktype(L, -1, LUA_TTABLE);
   lua_len(L, -1);//push len
   count = (int)luaL_checkinteger(L, -1);
   lua_pop(L, 1);//pop the len

   //kill any old members of the set
   dbTileSchemaDeleteByset(view->db, c_str(setName));

   for (i = 0; i < count; ++i) {
      DBTileSchema newTile = { 0 };
      newTile.set = stringCopy(setName);

      lua_pushinteger(L, i + 1);
      lua_gettable(L, -2);//push currnet tile
      luaL_checktype(L, -1, LUA_TTABLE);

      lua_pushliteral(L, "sprite");
      lua_gettable(L, -2);//push sprite
      newTile.sprite = stringCreate(luaL_checkstring(L, -1));
      lua_pop(L, 1);//pop sprite

      lua_pushliteral(L, "occlusion");
      lua_gettable(L, -2);//push 
      if (lua_isboolean(L, -1)) { newTile.occlusion = lua_toboolean(L, -1); }
      lua_pop(L, 1);//pop 

      lua_pushliteral(L, "lit");
      lua_gettable(L, -2);//push 
      if (lua_isboolean(L, -1)) { newTile.lit = lua_toboolean(L, -1); }
      lua_pop(L, 1);//pop 

      if (newTile.lit) {
         lua_pushliteral(L, "radius");
         lua_gettable(L, -2);//push 
         if (lua_isinteger(L, -1)) { newTile.radius = (int)lua_tointeger(L, -1); }
         lua_pop(L, 1);//pop 

         lua_pushliteral(L, "fadeWidth");
         lua_gettable(L, -2);//push 
         if (lua_isinteger(L, -1)) { newTile.fadeWidth = (int)lua_tointeger(L, -1); }
         lua_pop(L, 1);//pop 

         lua_pushliteral(L, "centerLevel");
         lua_gettable(L, -2);//push 
         if (lua_isinteger(L, -1)) { newTile.centerLevel = (int)lua_tointeger(L, -1); }
         lua_pop(L, 1);//pop 
      }


      lua_pop(L, 1);//pop the current frame
      dbTileSchemaInsert(view->db, &newTile);
      dbTileSchemaDestroy(&newTile);
   }

   lua_pop(L, 1);//pop the frames table
   stringDestroy(setName);
   return 0;

}


