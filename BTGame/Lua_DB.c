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


void luaLoadDBLibrary(lua_State *L) {

   lua_newtable(L);
   luaPushFunctionTable(L, "insertImage", &slua_DBInsertImage);
   luaPushFunctionTable(L, "insertImageFolder", &slua_DBInsertImageFolder);
   luaPushFunctionTable(L, "insertPalette", &slua_DBInsertPalette);
   luaPushFunctionTable(L, "insertPaletteFolder", &slua_DBInsertPaletteFolder);
   luaPushFunctionTable(L, "insertSprite", &slua_DBInsertSprite);

   lua_setglobal(L, LLIB_DB);

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
   newSprite.width = luaL_checkinteger(L, -1);
   lua_pop(L, 1);//pop width

   //height
   lua_pushinteger(L, 2);
   lua_gettable(L, -2);//push height
   newSprite.height = luaL_checkinteger(L, -1);
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
   fCount = luaL_checkinteger(L, -1);
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
      newFrame.image = stringCreate(luaL_checkstring(L, -1));
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
      newFrame.imgX = luaL_checkinteger(L, -1);
      lua_pop(L, 1);//pop x

      //y
      lua_pushinteger(L, 2);
      lua_gettable(L, -2);//push y
      newFrame.imgY = luaL_checkinteger(L, -1);
      lua_pop(L, 1);//pop y

      lua_pop(L, 1);//pop the pos table
      lua_pop(L, 1);//pop the current frame
      dbSpriteFrameInsert(view->db, &newFrame);
      dbSpriteFrameDestroy(&newFrame);
   }

   lua_pop(L, 1);//pop the frames table

   dbSpriteInsert(view->db, &newSprite);
   dbSpriteDestroy(&newSprite);

   return 0;
}

