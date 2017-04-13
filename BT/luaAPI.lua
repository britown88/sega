
-- standard.lua
Global
   new(self, out) -- creates out and sets the metatable and index to that of self
   cls() -- hosrthand for console.clear()
   dir(table) -- lists members of a table
   reload(module) -- unloads a required module and requires it

-- actors.lua
Global
   actors:[actor] -- list of all actors automatically created by the engine
      add(actor) -- adds an actor object to the global list (will skip if its already in)
      remove(actor) -- removes an actor from the global list (by comparing the Actor* udata)
      get(actor) -- returns the actor object from the global list that has the same Actor* udata as the input
      indexOf(actor) -- returns the 1-based index in the global list of an actor with the Actor* udata of the input (0 for not found)

-- actions.lua
Global
   actions --coroutine functions for scripting
      waitForStop(actor) -- yields until gridsnapped and stopped      
      move(actor, x, y) -- attempt to move to (x, y) and yields until it reaches it, stops, or cant make it
      persistentMove(actor, destX, destY, range) -- continues to call .move until within range of (destX, destY)      
      moveRelative(actor, x, y) -- move (x, y) offset from starting position
      wait(actor, seconds) -- yield for a given number of seconds 
      stop(actor) -- cancel move and wait to stop
      wander(actor, width, height) -- move between random locations within a square (width, height) centered on the start position
      scatter(actor) -- test: scatter all actors



-- Lua_StandardLibrary
Global   
   rand(lower, upper) -- random number inclusive lower, exclusive upper
   toggleStats() -- toggle Framerate view
   openEditor() -- open map editor
   setPalette(paletteID)  --changes palette. can use either a string literal or a udata of the stringview
   toggleLightMode() -- lighting debugging

   -- for testing.  prints an intellisense list from the input table
   intellij(table)

   console
      print(string) -- print string to console
      clear() -- clear console

   img
      clearImageCache() -- forces images to reload from DB
      clearSpriteCache() -- forces sprites to reload from DB

-- Lua_UILibrary
Global
   -- prompt the user with a set of string choices
   -- yields until the user selects
   -- Returns (resultIndex, resultString)
   promptChoices(stringList)

   textAreas:[textArea] -- list of text areas

   textArea
      push(self, string) -- pushes message to messagestack
      hide(self) -- hide text area
      show(self) -- show hidden text area
      wait(self) -- starts coroutine that will continue to yield until current messagequeue is empty
      setVisibility(self, show:bool) -- hide/show text area

-- Lua_Map
Global
   map
      new(width, height) -- creates and loads an empty new map of the given size
      resize(width, height) -- resizes the current map in memory
      load(path) -- loads a map at given path
      save(path) -- saves the currently loaded map to the path
      setTileSchema(set:string) -- sets current tileschema to set from db
      setAmbient(int) -- sets current ambient light level

-- Lua_DB
Global
   db
      insertImage(id, path) -- inserts a new image into the db, id is string
      insertImageFolder(path) -- attempt to call InsertImage on all (*.ega) files in the path (uses filename for id)
      insertPalette(id, path) -- inserts a new palette into the db, id is string
      insertPaletteFolder(path) -- attempt to call insertPalette on all (*.pal) files in the path (uses filename for id)
      insertSprite(sprite) -- inserts a new sprite: {id:string, size:{width, height}, frames:[{image:string, pos:{x, y}}]}
      insertTileSchema(schema) -- inserts new schema: {set:string, tiles:[{sprite:string, occlusion:bool, lit:bool, <radius:int, fadeWidth:int, centerLevel:int>}]} //3 lighting properties onlyt for lit
      insertLuaScript(modName, path) -- reads .lua file at path and loads it into the database under a given module name
      insertMap(name, mapPath) -- adds a loaded raw map to the db
      beginTransaction() -- call BEGIN TRANSACTION on db
      endTransaction() -- call END TRANSACTION on db


-- Lua_ActorLibrary
Global
   actor
      actor -- userdata for the Actor*

      scripts:[thread] -- Scripts stack

      response:{toLook:function(actor), toTalk:function(actor), toUse:function(actor), toFight:function(actor)}

      pushScript(self, function(actor)) --add a new script(actor) function and run it
      popScript(self) -- removes the topmost script from the scripts stack
      stepScript(self) -- coroutine.resume() on top script

      move(self, x, y) -- sets movement destination for pathfinding
      teleport(self, x, y) -- snaps to grid at (x,y)
      moveRelative(self, x, y) -- move x,y relative to current position
      position(self) -- returns (x, y) current position
      stop(self) -- stops current destination
      isMoving(self) -- returns boolean true/false
      distanceTo(self, x, y) -- takes target position and returns integer distance
      moveSpeed(self) -- returns (moveTime, moveDelay)
      setMoveSpeed(self, [{time, delay}, (time, delay)]) -- takes either as a table or two integers


-- Lua_Time
Global
   time
      jumpMinutes(minutes) -- set caelndar target forward
      jumpHours(hours) -- set caelndar target forward
      jumpDays(days) -- set caelndar target forward
      jumpWeeks(weeks) -- set caelndar target forward
      jumpMonths(months) -- set caelndar target forward
      jumpSeasons(seasons) -- set caelndar target forward
      jumpYears(years) -- set caelndar target forward

      pause() -- pause execution of the calendar
      resume() -- resume calendar execution






