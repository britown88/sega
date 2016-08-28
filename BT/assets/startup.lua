--openEditor()
map.setTileSchema("default")

loadMap 'lighttest'
map.setAmbient(0)
setPalette 'default'
player:teleport(6, 5)
--toggleStats();


--require 'lua.test'
--player:pushScript(testText)

toggleLightMode()
openEditor()
--map.setAmbient(0)
